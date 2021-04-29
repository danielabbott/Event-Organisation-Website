/*
Functionality related to user accounts (account creation, password resets, sessions, etc.)

*/

#pragma once 

#include <cstdint>
#include <string>
#include <istream>
#include <optional>
#include <array>
#include "IP.h"
#include "ErrorMaybe.h"
#include "DB/Media.h"

namespace Users {

constexpr const unsigned PASSWORD_RESET_CODE_LENGTH = 16;

struct PasswordHash {
	union {
		struct {
			uint32_t meta;
			uint8_t hash[32];
			uint8_t salt[16];
		};
		uint8_t bytes[52];
	};

	PasswordHash();
	PasswordHash(uint32_t meta, std::string const& password);

	bool password_matches_hash(std::string const& password) const;

	void to_string(char s[64]) const;
};

// Times the hashing function
long hash_time(uint32_t meta);

using Name = std::string;
using ID = uint32_t;

class SessionToken {
public:
	// This is hardcoded as 16 in some places
	// Server.cpp has a string which assumes this is 16
	static constexpr int LENGTH = 16;

private:
	uint8_t bytes[LENGTH];

public:


	// Generates unique ID and stores in database
	SessionToken(uint32_t user_id, IPv6Address ip);

	// LENGTH*2 characters
	void to_string(char *) const;

	const uint8_t * get_bytes() const { return &bytes[0]; }


	SessionToken(const SessionToken&) = delete;
	SessionToken& operator=(const SessionToken&) = delete;
	SessionToken(SessionToken&& other) = default;
	SessionToken& operator=(SessionToken&& other) = default;
};

ErrorMaybe<std::pair<ID, SessionToken>> create_account(
	std::string const& email, Name const& name, std::string const& password, IPv6Address ip);

ErrorMaybe<std::tuple<ID, Name, SessionToken>> log_in_to_account(
	std::string const& email, std::string const& password, IPv6Address ip);

void delete_session(SessionToken && token);

// Returns true if session was found and deleted
bool delete_session(const char * token);

std::optional<ID> get_user_id_from_session(const char * token);


struct UserBasicInfo {
	Users::ID id;
	Users::Name name;
	std::optional<MediaID> profile_picture_id;
	std::string email;
	int privilege_level;
};

// ID, Name, profile picture ID, email
std::optional<UserBasicInfo> get_user_basic_info_from_session(const char * token);

std::optional<std::pair<ID, Name>> get_user_basic_info_from_email(std::string const& email);

// Creates the code and stores it in the database
void create_password_reset_code(std::array<char, PASSWORD_RESET_CODE_LENGTH*2> & code, ID, std::string const& email);

// Returns [expires_in_x_minutes, email]
std::optional<std::pair<unsigned int, std::string>> verify_password_reset_code(std::string const& code);

// Returns user id on success
std::optional<ID> reset_password(std::string const& code, std::string const& password);

};
