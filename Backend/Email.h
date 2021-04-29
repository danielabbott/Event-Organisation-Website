#pragma once

#include <string>
#include <memory>
#include <array>
#include <vector>
#include <unordered_set>
#include "Users.h"

namespace Emails {

struct Email {
	std::string to;

	// Makes this class dynamic
	virtual ~Email() {}

protected:
	Email(std::string && to_) : to(move(to_)) {}
};

struct SignUpEmail : public Email {
	std::string name;


	SignUpEmail(std::string && to_, std::string && name_) : Email(move(to_)), name(move(name_)){};
};

struct PasswordResetEmail : public Email {
	std::string name;
	
	// in hexadecimal
	std::array<char, Users::PASSWORD_RESET_CODE_LENGTH*2> unique_code;

	PasswordResetEmail(std::string && to_, std::string && name_, std::array<char, Users::PASSWORD_RESET_CODE_LENGTH*2> && unique_code_)
	 : Email(move(to_)), name(move(name_)), unique_code(move(unique_code_)){};
};

struct InviteEmail : public Email {
	std::string event_name;
	uint32_t event_id;


	InviteEmail(std::string && to_, std::string && event_name_, uint32_t event_id_)
	 : Email(move(to_)), event_name(move(event_name_)), event_id(event_id_) {};
};

struct UnreadNotificationsEmail : public Email {
	std::string name;

	UnreadNotificationsEmail(std::string && to_, std::string && name_)
	 : Email(move(to_)), name(name_) {};
};

void start_thread();
bool email_is_valid(const char *);
void send_email(std::unique_ptr<Email>&&);
void send_emails(std::vector<std::unique_ptr<Email>> & emails);
void stop_thread();

void send_invitation_emails(std::string && event_name, uint32_t event_id, std::vector<std::string> const& email_addresses);
void send_invitation_emails(std::string && event_name, uint32_t event_id, std::unordered_set<std::string> const& email_addresses);

}