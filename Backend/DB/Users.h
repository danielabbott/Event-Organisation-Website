#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <variant>
#include "../Users.h"
#include "../IP.h"
#include "../ErrorMaybe.h"
#include "Media.h"

namespace DB {


// Use the functions in Users.h, these functions don't create sessions

ErrorMaybe<Users::ID> create_account(std::string const& email, std::string const& name, std::string const& password, IPv6Address ip);
ErrorMaybe<std::pair<Users::ID, Users::Name>> log_in_to_account(std::string const& email, std::string const& password);

void try_create_session(Users::SessionToken const&, Users::ID, IPv6Address const&);


void delete_session(Users::SessionToken &&);

bool delete_session(uint8_t token[Users::SessionToken::LENGTH]);

std::optional<Users::ID> get_user_id_from_session(uint8_t token[Users::SessionToken::LENGTH]);


std::optional<Users::UserBasicInfo> get_user_basic_info_from_session(uint8_t token[Users::SessionToken::LENGTH]);

std::optional<std::pair<Users::ID, Users::Name>> get_user_basic_info_from_email(std::string const& email);

std::optional<std::string> get_users_name(Users::ID);



void create_password_reset_code(uint8_t bytes[Users::PASSWORD_RESET_CODE_LENGTH], Users::ID, std::string const& email);

// Returns [expires_in_x_minutes, email]
std::optional<std::pair<unsigned int, std::string>> verify_password_reset_code(uint8_t bytes[Users::PASSWORD_RESET_CODE_LENGTH]);

std::optional<Users::ID> reset_password(uint8_t bytes[Users::PASSWORD_RESET_CODE_LENGTH], std::string const& password);

// name, bio, profile_picture_id, privilege_level, following
ErrorMaybe<std::tuple<Users::Name, std::string, std::optional<MediaID>, int>> get_user_page_info(Users::ID);

ErrorType save_user_profile(Users::ID, Users::Name, std::string bio);

ErrorMaybe<std::pair<MediaID, std::string>> set_profile_picture(Users::ID logged_in_user_id, std::string file_name);
ErrorType delete_profile_picture(Users::ID logged_in_user_id);

ErrorType set_user_privilege(Users::ID logged_in_user_id, Users::ID to_change, uint8_t change_to);

ErrorType follow_user(Users::ID logged_in_user_id, Users::ID to_follow);
void unfollow_user(Users::ID logged_in_user_id, Users::ID to_unfollow);
bool is_user_following(Users::ID follower, Users::ID followed);
std::vector<Users::ID> get_followers(Users::ID);

}