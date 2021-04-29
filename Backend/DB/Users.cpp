#include "DB_.h"
#include "../AWS.h"

namespace DB {
	
ErrorMaybe<Users::ID> create_account(string const& email, string const& name, string const& password, IPv6Address ip) {
	auto connection_data = open_database();

	// Get number of accounts made today
	*connection_data.ip = ip;
	connection_data.stmnt_antispam_get_accounts_created->set_data(0, connection_data.d_ref_ip);
	auto result = connection_data.stmnt_antispam_get_accounts_created->query();

	if(!result->next()) { // First account made today from this IP
		connection_data.stmnt_antispam_new_ip->set_data(0, connection_data.d_ref_ip);
		connection_data.stmnt_antispam_new_ip->execute();
	}
	else {
		auto max_accounts = stoi(get_setting(Setting::max_accounts_per_ip_per_day));
		if(result->get_unsigned16("accounts_created") >= max_accounts) {
			return ErrorType::TooManyRequests;
		}
	}
	// Increase counter by 1
	connection_data.stmnt_antispam_account_created->set_data(0, connection_data.d_ref_ip);
	connection_data.stmnt_antispam_account_created->execute();

	connection_data.stmnt_add_user->set_string(0, email.c_str());
	connection_data.stmnt_add_user->set_string(1, name.c_str());

	*connection_data.pwd_hash = Users::PasswordHash(0, password);

	// char s[65];
	// connection_data.pwd_hash->to_string(s);
	// s[64]=0;
	// cout <<s<<'\n';

	connection_data.stmnt_add_user->set_data(2, connection_data.d_ref_hash);

	uint64_t row_id = connection_data.stmnt_add_user->insert();
	if(row_id) return row_id;
	return ErrorType::GenericError;
}

ErrorMaybe<pair<Users::ID, Users::Name>> log_in_to_account(string const& email, string const& password) {
	auto connection_data = open_database();

	connection_data.stmnt_get_user0->set_string(0, email.c_str());
	auto result = connection_data.stmnt_get_user0->query();
	if (!result->next()) {
		return ErrorType::InvalidParameter;
	}

	data_ref dr = result->get_data("password");

	assert_(dr != nullptr && dr->size() == 52);

	Users::PasswordHash const& correct_password_hash = *reinterpret_cast<Users::PasswordHash*>(dr->get());

	if(correct_password_hash.password_matches_hash(password)) {
		return pair<Users::ID, Users::Name>(result->get_unsigned32("id"), result->get_string("name"));
	}
	else {
		return ErrorType::PermissionsError;
	}
}

void try_create_session(Users::SessionToken const& token, Users::ID user_id, IPv6Address const& ip)
{
	auto connection_data = open_database();
	connection_data.stmnt_create_session->set_unsigned32(0, user_id);

	*connection_data.ip = ip;
	connection_data.stmnt_create_session->set_data(1, connection_data.d_ref_ip);

	memcpy(connection_data.d_ref_session_token->get(), token.get_bytes(), Users::SessionToken::LENGTH);
	connection_data.stmnt_create_session->set_data(2, connection_data.d_ref_session_token);

	connection_data.stmnt_create_session->insert();
}

void delete_session(Users::SessionToken && token) {
	auto connection_data = open_database();

	memcpy(connection_data.d_ref_session_token->get(), token.get_bytes(), Users::SessionToken::LENGTH);
	connection_data.stmnt_delete_session->set_data(0, connection_data.d_ref_session_token);

	connection_data.stmnt_delete_session->execute();
}

bool delete_session(uint8_t token[Users::SessionToken::LENGTH])
{
	auto connection_data = open_database();

	memcpy(connection_data.d_ref_session_token->get(), token, Users::SessionToken::LENGTH);
	connection_data.stmnt_delete_session->set_data(0, connection_data.d_ref_session_token);

	return connection_data.stmnt_delete_session->execute() != 0;
}

optional<Users::ID> get_user_id_from_session(uint8_t token[Users::SessionToken::LENGTH])
{
	auto connection_data = open_database();

	memcpy(connection_data.d_ref_session_token->get(), token, Users::SessionToken::LENGTH);
	connection_data.stmnt_get_user_id_from_session->set_data(0, connection_data.d_ref_session_token);

	auto result = connection_data.stmnt_get_user_id_from_session->query();

	if (result->next()) {
		return result->get_unsigned32("user_id");
	}
	return nullopt;
}

optional<string> get_users_name(Users::ID id)
{
	auto connection_data = open_database();

	connection_data.stmnt_get_user_name_from_id->set_unsigned32(0, id);
	auto result = connection_data.stmnt_get_user_name_from_id->query();

	if (result->next()) {
		return result->get_string("name");
	}
	return nullopt;
}

optional<Users::UserBasicInfo> get_user_basic_info_from_session(uint8_t token[Users::SessionToken::LENGTH]) {
	auto connection_data = open_database();

	memcpy(connection_data.d_ref_session_token->get(), token, Users::SessionToken::LENGTH);
	connection_data.stmnt_get_user_info_from_session->set_data(0, connection_data.d_ref_session_token);

	auto result = connection_data.stmnt_get_user_info_from_session->query();

	if (result->next()) {		
		optional<MediaID> ppic;

		if(!result->get_is_null("profile_picture_id")) {
			MediaID m;
			data_ref dr = result->get_data("profile_picture_id");
			memcpy(m.data, dr->get(), 17);
			ppic = m;
		}
		
		return Users::UserBasicInfo {
			result->get_unsigned32("id"),
			result->get_string("name"),
			ppic,
			result->get_string("email"),
			result->get_unsigned8("privilege_level")
		};
	}
	return nullopt;
}

optional<pair<Users::ID, Users::Name>> get_user_basic_info_from_email(string const& email)
{
	auto connection_data = open_database();

	connection_data.stmnt_get_user_basic_info_from_email->set_string(0, email);

	auto result = connection_data.stmnt_get_user_basic_info_from_email->query();

	if (result->next()) {		
		return pair<Users::ID, Users::Name>(
			result->get_unsigned32("id"),
			result->get_string("name")
		);
	}
	return nullopt;
}

void create_password_reset_code(uint8_t bytes[Users::PASSWORD_RESET_CODE_LENGTH], Users::ID user_id, string const& email) {
	auto connection_data = open_database();
	connection_data.stmnt_create_password_reset->set_unsigned32(0, user_id);

	memcpy(connection_data.d_ref_pwd_reset_code->get(), bytes, Users::PASSWORD_RESET_CODE_LENGTH);
	connection_data.stmnt_create_password_reset->set_data(1, connection_data.d_ref_pwd_reset_code);

	connection_data.stmnt_create_password_reset->set_string(2, email);

	connection_data.stmnt_create_password_reset->insert();
}

optional<pair<unsigned int, string>> verify_password_reset_code(uint8_t bytes[Users::PASSWORD_RESET_CODE_LENGTH])
{
	auto connection_data = open_database();
	
	// Clear out old codes
	connection_data.stmnt_delete_password_resets->execute();

	// Find this code (if it exists)

	memcpy(connection_data.d_ref_pwd_reset_code->get(), bytes, Users::PASSWORD_RESET_CODE_LENGTH);
	connection_data.stmnt_get_password_reset->set_data(0, connection_data.d_ref_pwd_reset_code);

	auto result = connection_data.stmnt_get_password_reset->query();

	if (result->next()) {
		int64_t t_delta = result->get_signed64("t_delta"); // In seconds

		if(t_delta >= 0 && t_delta <= 10*60) {
			return pair<unsigned int, string>(
				(10*60-t_delta)/60,
				result->get_string("email")
			);
		}
	}
	return nullopt;
}

optional<Users::ID> reset_password(uint8_t bytes[Users::PASSWORD_RESET_CODE_LENGTH], string const& password)
{
	auto connection_data = open_database();
	connection_data.stmnt_delete_password_resets->execute();

	memcpy(connection_data.d_ref_pwd_reset_code->get(), bytes, Users::PASSWORD_RESET_CODE_LENGTH);
	connection_data.stmnt_do_password_reset1->set_data(0, connection_data.d_ref_pwd_reset_code);
	auto result = connection_data.stmnt_do_password_reset1->query();

	if (result->next()) {
		uint32_t user_id = result->get_unsigned32("user_id");
		*connection_data.pwd_hash = Users::PasswordHash(0, password);
		connection_data.stmnt_do_password_reset2->set_data(0, connection_data.d_ref_hash);
		connection_data.stmnt_do_password_reset2->set_unsigned32(1, user_id);
		auto rows = connection_data.stmnt_do_password_reset2->execute();

		connection_data.stmnt_kill_sessions->set_unsigned32(0, user_id);
		connection_data.stmnt_kill_sessions->execute();

		if(rows == 0) {
			return nullopt;
		}
		return user_id;
	}
	else {
		return nullopt;
	}
}


ErrorMaybe<tuple<Users::Name, string, optional<MediaID>, int>> get_user_page_info(Users::ID id)
{
	auto connection_data = open_database();

	connection_data.stmnt_get_user_profile->set_unsigned32(0, id);

	auto result = connection_data.stmnt_get_user_profile->query();

	if (!result->next()) {	
		return ErrorType::NotFound;
	}	

	optional<MediaID> ppic;

	if(!result->get_is_null("profile_picture_id")) {
		MediaID m;
		data_ref dr = result->get_data("profile_picture_id");
		memcpy(m.data, dr->get(), 17);
		ppic = m;
	}

	return tuple<Users::Name, string, optional<MediaID>, int>(
		result->get_string("name"),
		result->get_string("bio"),
		ppic,
		result->get_unsigned8("privilege_level")
	);

}

ErrorType save_user_profile(Users::ID user_id, Users::Name name, std::string bio)
{
	auto connection_data = open_database();

	connection_data.stmnt_save_user_profile->set_string(0, name);
	connection_data.stmnt_save_user_profile->set_unsigned32(2, user_id);

	if(bio.size() == 0) {
		connection_data.stmnt_save_user_profile->set_null(1);
	}
	else {
		connection_data.stmnt_save_user_profile->set_string(1, bio);
	}

	auto affected = connection_data.stmnt_save_user_profile->execute();

	if (!affected) {	
		return ErrorType::GenericError;
	}	

	return ErrorType::Success;
}


bool try_add_media(optional<EventID> event_id, string file_name, bool is_cover_image);

ErrorMaybe<pair<MediaID, S3PresignedURL>> set_profile_picture(Users::ID logged_in_user_id, string file_name)
{
	auto connection_data = open_database();
	

	MediaID& m_id = *connection_data.media_id;
	bool got_id = try_add_media(nullopt, file_name, false);

	if(!got_id) {
		return ErrorType::GenericError;
	}

	connection_data.stmnt_set_ppic->set_data(0, connection_data.d_ref_media_id);
	connection_data.stmnt_set_ppic->set_unsigned32(1, logged_in_user_id);
	connection_data.stmnt_set_ppic->execute();


	return pair<MediaID, S3PresignedURL>(m_id, put_s3_object(m_id.to_string(), file_name));
}

ErrorType delete_profile_picture(Users::ID logged_in_user_id)
{
	auto connection_data = open_database();

	connection_data.stmnt_get_ppic->set_unsigned32(0, logged_in_user_id);
	auto result = connection_data.stmnt_get_ppic->query();
	if(!result->next()) return ErrorType::NotFound;

	if(result->get_is_null("profile_picture_id")) {
		return ErrorType::Success;
	}


	auto pid_ = result->get_data("profile_picture_id");
	auto& pid = *reinterpret_cast<MediaID*>(pid_->get());


	connection_data.stmnt_set_ppic->set_null(0);
	connection_data.stmnt_set_ppic->set_unsigned32(1, logged_in_user_id);
	connection_data.stmnt_set_ppic->execute();

	connection_data.stmnt_delete_media->set_data(0, pid_);
	connection_data.stmnt_delete_media->execute();

	delete_s3_object(pid.to_string());

	return ErrorType::Success;
}

ErrorType set_user_privilege(Users::ID logged_in_user_id, Users::ID to_change, uint8_t change_to)
{
	auto connection_data = open_database();

	if(change_to > 1) {
		return ErrorType::PermissionsError;
	}

	{
		connection_data.stmnt_get_user_privilege->set_unsigned32(0, logged_in_user_id);
		auto result = connection_data.stmnt_get_user_privilege->query();
		if(!result->next()) return ErrorType::PermissionsError;
		if(result->get_unsigned8("privilege_level") != 2) {
			return ErrorType::PermissionsError;
		}
	}

	connection_data.stmnt_set_user_privilege->set_unsigned8(0, change_to);
	connection_data.stmnt_set_user_privilege->set_unsigned32(1, to_change);

	connection_data.stmnt_set_user_privilege->execute();


	return ErrorType::Success;
}

ErrorType follow_user(Users::ID logged_in_user_id, Users::ID to_follow)
{
	auto connection_data = open_database();
	connection_data.stmnt_add_user_follow->set_unsigned32(0, to_follow);
	connection_data.stmnt_add_user_follow->set_unsigned32(1, logged_in_user_id);

	connection_data.stmnt_add_user_follow->execute();
	return ErrorType::Success;
}

void unfollow_user(Users::ID logged_in_user_id, Users::ID to_unfollow)
{
	auto connection_data = open_database();
	connection_data.stmnt_remove_user_follow->set_unsigned32(0, to_unfollow);
	connection_data.stmnt_remove_user_follow->set_unsigned32(1, logged_in_user_id);

	connection_data.stmnt_remove_user_follow->execute();
}

bool is_user_following(Users::ID follower, Users::ID followed)
{
	if(followed == follower) {
		return false;
	}

	auto connection_data = open_database();
	connection_data.stmnt_is_following->set_unsigned32(0, followed);
	connection_data.stmnt_is_following->set_unsigned32(1, follower);

	auto result = connection_data.stmnt_is_following->query();
	if(!result->next() || result->get_unsigned64("COUNT(*)") == 0) {
		return false;
	}
	return true;
}

vector<Users::ID> get_followers(Users::ID user)
{
	auto connection_data = open_database();

	connection_data.stmnt_get_followers->set_unsigned32(0, user);
	auto result = connection_data.stmnt_get_followers->query();

	vector<Users::ID> users;

	while (result->next()) {
		users.push_back(result->get_unsigned32("follower_user_id"));
	}

	return users;
}

}