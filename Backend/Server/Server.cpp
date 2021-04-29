#include "Server.h"

#include <httplib.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include "../DB/ConnLostHandle.h"

using namespace std;
using namespace httplib;

namespace WebServer {


bool has_param(RequestHandle req_, string const& name)
{
	const Request& req = *reinterpret_cast<const Request*>(req_);
	return req.has_param(name.c_str());
}

string get_param_value(RequestHandle req_, string const& name)
{
	const Request& req = *reinterpret_cast<const Request*>(req_);
	return req.get_param_value(name.c_str());
}

unsigned int has_header(RequestHandle req_, string const& name)
{
	const Request& req = *reinterpret_cast<const Request*>(req_);
	// return req.has_header(name.c_str());
	return req.get_header_value_count(name.c_str());
}

string get_header_value(RequestHandle req_, string const& name, unsigned int i)
{
	const Request& req = *reinterpret_cast<const Request*>(req_);
	return req.get_header_value(name.c_str(), i);
}

void set_header(ResponseHandle res_, string const& name, string const& value)
{
	Response& res = *reinterpret_cast<Response*>(res_);
	res.set_header(name.c_str(), value.c_str());
}


Resource::CallbackReturn response__hello(RequestHandle req, ResponseHandle res)
{
	(void)res;
	string s("hello");
	if(has_param(req, "name")) {
		s.append(", ");
		s.append(get_param_value(req, "name"));
	}
	return {200, s, MimeType::ASCII};
}


Resource::CallbackReturn response__my_ip(RequestHandle req, ResponseHandle res)
{
	(void)res;
	return {200, get_header_value(req, "X-Real-IP"), MimeType::ASCII};
}

void start_server() {
	Server svr;

	auto make_response = [](Response& res, 
			ReponseCode status, std::string const& data, MimeType mime_type) {
		res.status = status;
		const char * mime_type_string = 
			mime_type == MimeType::ASCII ? "text/plain" : "application/json; charset=UTF-8";
		if(data.size() > 0)
			res.set_content(data, mime_type_string);
	};

	auto do_response = [make_response](Resource const& rsrc, const Request& req, Response& res) {
		auto [status, data, mime_type] = db_handle_connection_fail(rsrc.callback, 
			reinterpret_cast<uintptr_t>(&req), reinterpret_cast<uintptr_t>(&res)
		);
		make_response(res, status == 404 ? 400 : status, data, mime_type);
	};

	for(Resource const& rsrc : resources) {
		if(rsrc.method == Method::Get) {
			svr.Get(rsrc.name.c_str(), [rsrc,do_response](const Request& req, Response& res) {
				do_response(rsrc, req, res);
			});
		}
		else {
			svr.Post(rsrc.name.c_str(), [rsrc,do_response](const Request& req, Response& res) {
				do_response(rsrc, req, res);
			});
		}
	}




	spdlog::info("HTTP server started");
	svr.listen("127.0.0.1", 8000);
}



void find_session_cookie(RequestHandle req_, char * session_string) {
	const Request& req = *reinterpret_cast<const Request*>(req_);

	*session_string = 0;
	if (req.has_header("Cookie")) {
		auto cookieCount = req.get_header_value_count("Cookie");
		for(unsigned int i = 0; i < cookieCount; i++) {
      		auto cookieHeader_ = req.get_header_value("Cookie", i);
      		const char * cookieHeader = cookieHeader_.c_str();

      		// Parse cookie header

      		while(*cookieHeader) {
      			while(*cookieHeader == ' ' || *cookieHeader == '\t') {
      				cookieHeader++;
      				continue;
      			}

      			if(!*cookieHeader) break;

      			if(cookieHeader[0] == 's' && cookieHeader[1] == '=') {
      				cookieHeader += 2;
      				auto session_cookie_ptr = cookieHeader;

      				for(int j = 0; j < Users::SessionToken::LENGTH*2; j++) {
      					if(!session_cookie_ptr[j] ||
      						!((session_cookie_ptr[j] >= 'A' && session_cookie_ptr[j] <= 'Z') || 
      							(session_cookie_ptr[j] >= 'a' && session_cookie_ptr[j] <= 'z') || 
      							(session_cookie_ptr[j] >= '0' && session_cookie_ptr[j] <= '9'))) {
      						// invalid code
      						return;
      					}
      				}
      				memcpy(session_string, session_cookie_ptr, Users::SessionToken::LENGTH*2);
      				return;
      			}
      			else {
		      		while(*cookieHeader && *cookieHeader != ';') {
	      				cookieHeader++;
		      		}
      			
		      		if(*cookieHeader) {

	      				cookieHeader++;
		      		}
      			}

      		}
      		
		}
    }
}


void log_out(RequestHandle req_)
{
	char session_token_string[Users::SessionToken::LENGTH*2];
	find_session_cookie(req_, session_token_string);
	if(session_token_string[0]) {
		Users::delete_session(session_token_string);
	}
}
	
// 30 days
static thread_local char cookie_string[] = "s=XXXXXXXXXXYYYYYYYYYYXXXXXXXXXXYY; Secure; HttpOnly, Max-Age=2592000";
// static thread_local char cookie_string[] = "s=XXXXXXXXXXYYYYYYYYYYXXXXXXXXXXYY; Max-Age=2592000";

void set_session_cookie(ResponseHandle res_, Users::SessionToken const& token)
{
	Response& res = *reinterpret_cast<Response*>(res_);
	token.to_string(&cookie_string[2]);
	res.set_header("Set-Cookie", cookie_string);
}

void clear_session_cookie(ResponseHandle res_) 
{
	Response& res = *reinterpret_cast<Response*>(res_);
	res.set_header("Set-Cookie", "s=; Secure; HttpOnly; Friday, 17-May-03 18:45:00 GMT");
	// res.set_header("Set-Cookie", "s=; Friday, 17-May-03 18:45:00 GMT");
}

optional<Users::ID> get_logged_in_user_id(RequestHandle req_, ResponseHandle res_) {
	char session_token_string[Users::SessionToken::LENGTH*2];
	find_session_cookie(req_, session_token_string);
	if(!session_token_string[0]) {
		return nullopt;
	}

	auto id = Users::get_user_id_from_session(session_token_string);
	if(!id.has_value()) {
		Users::delete_session(session_token_string);
		clear_session_cookie(res_);
		return nullopt;
	}

	return id;
};

void json_filter_string(stringstream & ss, string const& s)
{
	for(char c : s) {
		if(!c) {
			ss << "\\0";
		}
		else if(c == '\n') {
			ss << "\\n";
		}
		else if(c == '\r') {
			ss << "\\r";
		}
		else if(c == '\\') {
			ss << "\\\\";
		}
		else if(c == '"') {
			ss << "\\\"";
		}
		else if(c == '\b') {
			ss << "\\b";
		}
		else if(c == '\t') {
			ss << "\\t";
		}
		else if(c == 12) {
			ss << "\\f";
		}
		else {
			ss << c;
		}
	}
}

}