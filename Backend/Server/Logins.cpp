#include "Server.h"
#include "../Email.h"
#include "../DB/DB.h"
#include "../AWS.h"
#include <sstream>
#include <iostream>
#include <spdlog/spdlog.h>

using namespace std;

namespace WebServer {

Resource::CallbackReturn response__sign_up(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "email")) {
		return { 400, string("Missing parameter: email"), MimeType::ASCII };
	}
	if (!has_param(req, "pwd")) {
		return { 400, string("Missing parameter: pwd"), MimeType::ASCII };
	}
	if (!has_param(req, "name")) {
		return { 400, string("Missing parameter: name"), MimeType::ASCII };
	}

	string password = get_param_value(req, "pwd");
	if(password.size() < 8) {
		return { 400, string("Password too short"), MimeType::ASCII };
	}

	string email = get_param_value(req, "email");

	if(!Emails::email_is_valid(email.c_str())) {
		return { 400, string("Invalid email"), MimeType::ASCII };
	}

	log_out(req);


	string name = get_param_value(req, "name");
	string ip_string = get_header_value(req, "X-Real-IP");
	IPv6Address ip(ip_string);

	auto acc = Users::create_account(
		email, name, password, ip);


	if(!acc.has_value()) {
		clear_session_cookie(res);
		return { 500, string(), MimeType::ASCII };
	}

	spdlog::info("Account created: email [{}], name [{}], ip [{}]", email, name, ip_string);
	
	Emails::send_email(make_unique<Emails::SignUpEmail>(move(email), move(name)));

	auto const& [id, session_token] = acc.value();

	set_session_cookie(res, session_token);

	stringstream ss;
	ss << "{\"id\":" << to_string(id) << '}';
	return { 200, ss.str(), MimeType::JSON };
}


Resource::CallbackReturn response__log_in(RequestHandle req, ResponseHandle res)
{
	if (!has_param(req, "email")) {
		return { 400, string("Missing parameter: email"), MimeType::ASCII };
	}
	if (!has_param(req, "pwd")) {
		return { 400, string("Missing parameter: pwd"), MimeType::ASCII };
	}

	log_out(req);

	string email = get_param_value(req, "email");

	if(!Emails::email_is_valid(email.c_str())) {
		return { 400, string("Invalid Email"), MimeType::ASCII };
	}

	string ip_string = get_header_value(req, "X-Real-IP");
	IPv6Address ip(ip_string);
	auto user_data = Users::log_in_to_account(email, get_param_value(req, "pwd"), ip);

	if(!user_data.has_value()) {
		if(user_data.get_error() == ErrorType::PermissionsError) {
			spdlog::info("Log-in denied: email [{}], ip [{}]", email, ip_string);
		}
		else {
			spdlog::info("Log-in fail: email [{}], ip [{}]", email, ip_string);
		}
		clear_session_cookie(res);
		return { user_data.get_http_error(), string(), MimeType::ASCII };
	}

	auto const& [id, name, token] = user_data.value();

	spdlog::info("User logged in: id [{}], email [{}], ip [{}]", id, email, ip_string);

	set_session_cookie(res, token);


	stringstream ss;
	ss << "{\"name\":\"" << name << "\"}";
	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__my_basic_info(RequestHandle req, ResponseHandle res)
{
	char session_token_string[Users::SessionToken::LENGTH*2];
	find_session_cookie(req, session_token_string);
	if(!session_token_string[0]) {
		return { 401, string(), MimeType::ASCII };
	}

	auto ud_ = Users::get_user_basic_info_from_session(session_token_string);
	if(!ud_.has_value()) {
		clear_session_cookie(res);
		return { 401, string(), MimeType::ASCII };
	}
	auto const& user_data = ud_.value();

	stringstream ss;
	ss << "{\"id\":" << user_data.id << ",\"name\":\"";
	json_filter_string(ss, user_data.name);
	ss << "\",\"email\":\"";
	json_filter_string(ss, user_data.email);

	if(user_data.profile_picture_id.has_value()) {
		ss << "\", \"profPic\":\"" << user_data.profile_picture_id.value().to_string();
	}

	ss << "\",\"privilegeLevel\":" << user_data.privilege_level << "}";
	
	
	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__logout(RequestHandle req, ResponseHandle)
{
	log_out(req);
	return { 200, string(), MimeType::JSON };
}

Resource::CallbackReturn response__get_password_reset_link(RequestHandle req, ResponseHandle res)
{
	(void) res;

	if (!has_param(req, "email")) {
		return { 400, string("Missing parameter: email"), MimeType::ASCII };
	}


	string email = get_param_value(req, "email");

	if(!Emails::email_is_valid(email.c_str())) {
		return { 400, string("Invalid Email"), MimeType::ASCII };
	}

	auto ud = Users::get_user_basic_info_from_email(email);

	if(!ud.has_value()) {
		return { 400, string("Email address not associated with any account"), MimeType::ASCII };
	}

	auto & [id,name] = ud.value();

	string ip_string = get_header_value(req, "X-Real-IP");
	spdlog::info("Password reset link requested: email [{}], ip [{}]", email, ip_string);

	array<char, Users::PASSWORD_RESET_CODE_LENGTH*2> code;
	Users::create_password_reset_code(code, id, email);

	Emails::send_email(make_unique<Emails::PasswordResetEmail>(move(email), move(name), move(code)));
	return { 200, string(), MimeType::ASCII };
}

Resource::CallbackReturn response__verify_password_reset_link(RequestHandle req, ResponseHandle res)
{
	(void)res;

	if (!has_param(req, "code")) {
		return { 400, string("Missing parameter: code"), MimeType::ASCII };
	}

	auto x = Users::verify_password_reset_code(get_param_value(req, "code"));

	if(!x.has_value()) {
		return { 400, string("Invalid code"), MimeType::ASCII };
	}

	auto & [expires_in_x_minutes, email] = x.value();

	stringstream ss;
	ss << "{\"email\":\"" << email << "\",\"expiresIn\":" << expires_in_x_minutes << '}';
	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__password_reset(RequestHandle req, ResponseHandle res)
{
	(void)res;
	
	if (!has_param(req, "pwd")) {
		return { 400, string("Missing parameter: pwd"), MimeType::ASCII };
	}
	if (!has_param(req, "code")) {
		return { 400, string("Missing parameter: code"), MimeType::ASCII };
	}

	auto user_id = Users::reset_password(get_param_value(req, "code"), get_param_value(req, "pwd"));

	if(!user_id.has_value()) {
		return { 400, string(), MimeType::ASCII };
	}

	string ip_string = get_header_value(req, "X-Real-IP");
	spdlog::info("Password reset: id [{}], ip [{}]", user_id.value(), ip_string);
	return { 200, string(), MimeType::ASCII };
}

Resource::CallbackReturn response__user_profile(RequestHandle req, ResponseHandle res)
{
	(void)res;

	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	auto id = stoul(get_param_value(req, "id"));

	auto profile_info_ = DB::get_user_page_info(id);

	if(!profile_info_.has_value()) {
		return { profile_info_.get_http_error(), string(), MimeType::ASCII };
	}


	auto & [name, bio, profile_picture_id, privilege_level] = profile_info_.value();



	stringstream ss;
	ss << "{\"name\":\"";
	json_filter_string(ss, name);
	ss << "\",\"bio\":\"";
	json_filter_string(ss, bio);

	if(profile_picture_id.has_value()) {
		ss << "\", \"profPic\":\"" << profile_picture_id.value().to_string();
	}
	
	ss << "\", \"privilegeLevel\":" << privilege_level;



	auto my_id = get_logged_in_user_id(req, res);
	if (my_id.has_value() && my_id.value() != id) {
		bool following = DB::is_user_following(my_id.value(), id);
		ss << ", \"following\":" << (following ? "true" : "false") << "}";
	}
	else {
		ss << "}";
	}

	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__save_user_profile(RequestHandle req, ResponseHandle res)
{
	(void)res;

	if (!has_param(req, "name")) {
		return { 400, string("Missing parameter: name"), MimeType::ASCII };
	}
	if (!has_param(req, "bio")) {
		return { 400, string("Missing parameter: bio"), MimeType::ASCII };
	}

	auto name = get_param_value(req, "name");
	auto bio = get_param_value(req, "bio");

	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	return { get_http_error(DB::save_user_profile(my_id.value(), name, bio)), string(), MimeType::ASCII };
}

Resource::CallbackReturn response__new_profile_picture(RequestHandle req, ResponseHandle res)
{
	(void)res;

	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}


	if (!has_param(req, "file_name")) {
		return { 400, string("Missing parameter: file_name"), MimeType::ASCII };
	}

	DB::delete_profile_picture(my_id.value());

	string file_name = get_param_value(req, "file_name");

	auto x = DB::set_profile_picture(my_id.value(), file_name);
	if(!x.has_value()) {
		return { x.get_http_error(), string(), MimeType::ASCII };
	}

	auto const& [id, presigned_url] = x.value();


	stringstream ss;
	ss << "{\"id\":\"" << id.to_string() << "\", \"presignedURL\":\"" << presigned_url << "\"}";
	return { 200, ss.str(), MimeType::JSON };
}

Resource::CallbackReturn response__remove_profile_picture(RequestHandle req, ResponseHandle res)
{
	(void)res;

	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	DB::delete_profile_picture(my_id.value());

	ErrorType e = DB::delete_profile_picture(my_id.value());
	return { get_http_error(e), string(), MimeType::ASCII };
}


Resource::CallbackReturn response__set_user_priv(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}
	if (!has_param(req, "priv")) {
		return { 400, string("Missing parameter: priv"), MimeType::ASCII };
	}

	ErrorType e = DB::set_user_privilege(my_id.value(), stoi(get_param_value(req, "id")), stoi(get_param_value(req, "priv")));
	return { get_http_error(e), string(), MimeType::ASCII };
}

Resource::CallbackReturn response__follow(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	ErrorType e = DB::follow_user(my_id.value(), stoi(get_param_value(req, "id")));
	return { get_http_error(e), string(), MimeType::ASCII };
}

Resource::CallbackReturn response__unfollow(RequestHandle req, ResponseHandle res)
{
	auto my_id = get_logged_in_user_id(req, res);
	if (!my_id.has_value()) {
		return { 401, string(), MimeType::ASCII };
	}

	if (!has_param(req, "id")) {
		return { 400, string("Missing parameter: id"), MimeType::ASCII };
	}

	DB::unfollow_user(my_id.value(), stoi(get_param_value(req, "id")));
	return { 200, string(), MimeType::ASCII };
}

}
