#include "Email.h"
#include "Config.h"
#include <curl/curl.h>
#include <optional>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <iostream>
#include <mutex>
#include <queue>
#include <cstring>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>

using namespace std;

namespace Emails {

CURL* curl = nullptr;
atomic<bool> keep_running = true;

mutex queue_lock;
queue<unique_ptr<Email>> email_queue;

static void do_send_email(unique_ptr<Email>&&);
static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp);

static void connect_to_gmail() 
{
    if(curl) return;

    spdlog::info("Connecting to GMail..");

    curl = curl_easy_init();
    if(!curl) {
        throw runtime_error("curl_easy_init error");
    }

    curl_easy_setopt(curl, CURLOPT_USERNAME, get_setting(Setting::email_account).c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, get_setting(Setting::email_password).c_str());
    curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
    curl_easy_setopt(curl, CURLOPT_USE_SSL, static_cast<long>(CURLUSESSL_ALL));
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, get_setting(Setting::email_account).c_str());

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
}

void thread_function() {
    while(keep_running) {
    	queue<unique_ptr<Email>> emails_to_send;
    	{
			lock_guard<mutex> lock(queue_lock);
            emails_to_send = move(email_queue);
    	}
    	while(emails_to_send.size()) {
			do_send_email(move(emails_to_send.front()));
            emails_to_send.pop();
		}
    	this_thread::sleep_for(100ms);
    }
}

static thread email_thread;
void start_thread() {
	email_thread = thread(thread_function);
}

struct MessageSendState {
	string data;
	unsigned int data_sent = 0;

    MessageSendState(string && data_) : data(data_) {}
};



static void do_send_email(unique_ptr<Email>&& email) {
    spdlog::info("Sending email to {}", email->to);

    bool actually_send_email = true;

    string phony_email_server("@phonyemail.notreal");

    if(email->to.length() >= phony_email_server.length() &&
        email->to.compare(email->to.length() - phony_email_server.length(), 
                phony_email_server.length(), phony_email_server) == 0) {
        actually_send_email = false;
    }

    curl_slist* recipients = nullptr;
    if(actually_send_email) {
        connect_to_gmail();

    	recipients = curl_slist_append(nullptr, email->to.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    }

    stringstream ss;
    ss << "To: " << email->to << "\r\nFrom: " << get_setting(Setting::email_account) << 
        " (Event Organisation Website)\r\nSubject: ";


    auto signup_email = dynamic_cast<SignUpEmail*>(email.get());
    auto password_reset_email = dynamic_cast<PasswordResetEmail*>(email.get());
    auto invite_email = dynamic_cast<InviteEmail*>(email.get());
    auto unread_notifications_email = dynamic_cast<UnreadNotificationsEmail*>(email.get());


    if(signup_email) {
        ss << "Welcome, " << signup_email->name
        << "!\r\n\r\nWelcome, " << signup_email->name
        << ".\r\n\r\nYou are receiving this email because you recently created an account at "
            << get_setting(Setting::website_url) << '.';
    }
    else if (password_reset_email) {
        ss << "Password Reset\r\n\r\nHello, " << password_reset_email->name << ".\r\n\r\n"
        << "A password reset has been requested for your account. "
            "Either click the link below or copy-paste it into the address bar of your web browser to reset your password.\r\n\r\n" 
            << get_setting(Setting::website_url) << PASSWORD_RESET_URL_START
        << string(password_reset_email->unique_code.data(), password_reset_email->unique_code.size())
        << "\r\n\r\nIf you did not request this password reset then you can safely ignore this email. "
            "Your password will not be affected.";
    }
    else if(invite_email) {
        ss << "Event Invitation\r\n\r\nYou have been invited the event: " << invite_email->event_name
        << "\r\n\r\nYou can view details of the event and register your attendance at the following url:\r\n\r\n"
        << get_setting(Setting::website_url) << EVENT_URL_START
        << invite_email->event_id;
    }
    else if(unread_notifications_email) {
        ss << "Unread Notifications\r\n\r\nHello, "
        << unread_notifications_email->name
        << ". You have unread notifications at " << get_setting(Setting::website_url);
    }
    else {
        assert(false);
        return;
    }

    if(actually_send_email) {
        // Raw pointer gets freed in payload_source when last bit of data is sent
        auto message_state = new MessageSendState(ss.str());
        // email_data now invalid

        curl_easy_setopt(curl, CURLOPT_READDATA, message_state);

        auto e = curl_easy_perform(curl);
        if (e != CURLE_OK) {
            spdlog::error("curl_easy_perform() failed: {}", curl_easy_strerror(e));
        }

        // TODO: Can we avoid reallocating this each time?
        curl_slist_free_all(recipients);
    }
    else {
        spdlog::info("Email:\n{}", ss.str());
    }
}

void send_email(unique_ptr<Email>&& email) {
	lock_guard<mutex> lock(queue_lock);

	if(email_queue.size() >= 3000) {
		spdlog::error("Email Backlog >= 3000. Dropped email.");
		return;
	}

	email_queue.push(move(email));
}

void send_emails(vector<unique_ptr<Email>> & emails) {
    lock_guard<mutex> lock(queue_lock);

    if(email_queue.size() >= 3000) {
        spdlog::error("Email Backlog >= 3000. Dropped {} emails.", emails.size());
        return;
    }

    for(auto & e : emails) {
        email_queue.push(move(e));
    }
}

template <typename T>
void send_invitation_emails(string && event_name, uint32_t event_id, T const& email_addresses)
{
    vector<unique_ptr<Email>> emails;
    emails.reserve(email_addresses.size());

    for(string const& addr : email_addresses) {
        emails.push_back(make_unique<InviteEmail>(string(addr), move(event_name), event_id));
    }
    send_emails(emails);
}

void send_invitation_emails(string && event_name, uint32_t event_id, vector<string> const& email_addresses)
{
    send_invitation_emails<vector<string>>(move(event_name), event_id, email_addresses);
}

void send_invitation_emails(string && event_name, uint32_t event_id, unordered_set<string> const& email_addresses)
{
    send_invitation_emails<unordered_set<string>>(move(event_name), event_id, email_addresses);
}

static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp)
{
    auto state = reinterpret_cast<MessageSendState *>(userp);
    auto bytes_left = state->data.size() - state->data_sent;

    if(bytes_left == 0) return 0;

    size_t bytes_to_write = min(size*nmemb, bytes_left);

    memcpy(ptr, &state->data.c_str()[state->data_sent], bytes_to_write);
    state->data_sent += bytes_to_write;

    return bytes_to_write;
}

void stop_thread()
{
	keep_running = false;
}

// Very basic - checks if there is one and only one '@' and a '.' that comes after the '@''
bool email_is_valid(const char * s)
{
    bool atSymbolFound = false;
    bool dotSymbolFoundAfterAt = false;
    while(*s) {
        char c = *s;
        if(c == '@') {
            if(atSymbolFound) {
                return false;
            }
            atSymbolFound = true;
        }
        else if(c == '.') {
            if(atSymbolFound) {
                dotSymbolFoundAfterAt = true;
            }
        }
        else {
            static const string other_valid_chars("!#$%&'*+-/=?^_`{|}~");
            bool valid = 
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            other_valid_chars.find(c) != string::npos;

            if(!valid) {
                return false;
            }
        }

        s++;
    }
    return atSymbolFound && dotSymbolFoundAfterAt;
}
}