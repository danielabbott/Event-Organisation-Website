#include "Users.h"
#include <argon2.h>
#include "Assert.h"
#include <string>
#include <chrono>
#include <spdlog/spdlog.h>
#include "DB/Users.h"
#include "ArrayPointer.h"
#include "HexString.h"
#include "Random.h"

using namespace std;

namespace Users {

constexpr const char * PASSWORD_PEPPER = "\x9a\xee\xa1\x58\x96\x29\x67\xed\xd5\x07\xba\x11\x59\x1d";



static void do_hash(uint32_t meta, string pwd, uint8_t hash[32], const uint8_t salt[16]) {
	assert_(meta <= 1);

	uint32_t memory = 3*1024; // kilobytes of memory to use

	if (meta == 1) {
		memory = 48*1024;
	}

	pwd.append(PASSWORD_PEPPER);

	argon2id_hash_raw(
		2, // 1-pass
		memory, 
		1, // 1 thread
		pwd.c_str(), pwd.length(), salt, 16, hash, 32
	);
}



PasswordHash::PasswordHash(uint32_t meta_, string const& pwd)
:meta(meta_)
{
	try_create_random(salt, 16);
	do_hash(meta, pwd, hash, salt);	
}

PasswordHash::PasswordHash() {
	memset(bytes, 0, sizeof bytes);
}

void PasswordHash::to_string(char s[64]) const {
	bytes_to_hex_string(ArrayPointer<const uint8_t>(hash, sizeof hash), ArrayPointer<char>(s, 64));
}

bool PasswordHash::password_matches_hash(string const& password) const
{
	uint8_t hash2[32];
	do_hash(meta, password, hash2, salt);

	return memcmp(hash, hash2, 32) == 0;
}

long hash_time(uint32_t meta) {

	auto begin = chrono::steady_clock::now();
	uint8_t salt[16];
	uint8_t hash[32];
	do_hash(meta, "12345", hash, salt);
	auto end = chrono::steady_clock::now();
	return chrono::duration_cast<chrono::milliseconds>(end - begin).count();
}

SessionToken::SessionToken(uint32_t user_id, IPv6Address ip) {
	for(int attempts = 0; attempts < 7; attempts++) {
		try_create_random(bytes, LENGTH);

		try {
			DB::try_create_session(*this, user_id, ip);
			return;
		}
		catch (exception const& e){
			spdlog::error(e.what());
		}
	}
	assert_(false);
}

ErrorMaybe<pair<Users::ID, SessionToken>> create_account(
	string const& email, string const& name, string const& password, IPv6Address ip)
{
	auto id = DB::create_account(email, name, password, ip);
	if(!id.has_value()) return id.get_error();

	return pair<ID, SessionToken>(id.value(), SessionToken(id.value(), ip));
}

ErrorMaybe<tuple<ID, Name, SessionToken>> log_in_to_account(
	string const& email, string const& password, IPv6Address ip)
{
	auto ud = DB::log_in_to_account(email, password);
	if(!ud.has_value()) return ud.get_error();

	auto [id, name] = ud.value();

	return tuple<ID, Name, SessionToken>(id, name, SessionToken(id, ip));
}

void delete_session(SessionToken && token) {
	DB::delete_session(move(token));
}

void SessionToken::to_string(char * s) const {
	bytes_to_hex_string(ArrayPointer<const uint8_t>(bytes, LENGTH), ArrayPointer<char>(s, LENGTH*2));
}


bool delete_session(const char * str) {
	uint8_t token[SessionToken::LENGTH];
	hex_string_to_bytes(ArrayPointer<uint8_t>(token, SessionToken::LENGTH),
		ArrayPointer<const char>(str, SessionToken::LENGTH*2));
	

	return DB::delete_session(token);
}


optional<ID> get_user_id_from_session(const char * str)
{
	uint8_t token[SessionToken::LENGTH];
	hex_string_to_bytes(ArrayPointer<uint8_t>(token, SessionToken::LENGTH),
		ArrayPointer<const char>(str, SessionToken::LENGTH*2));

	return DB::get_user_id_from_session(token);
}

std::optional<UserBasicInfo> get_user_basic_info_from_session(const char * str)
{
	uint8_t token[SessionToken::LENGTH];
	hex_string_to_bytes(ArrayPointer<uint8_t>(token, SessionToken::LENGTH),
		ArrayPointer<const char>(str, SessionToken::LENGTH*2));

	return DB::get_user_basic_info_from_session(token);
}

optional<pair<ID, Name>> get_user_basic_info_from_email(string const& email)
{
	return DB::get_user_basic_info_from_email(email);
}

void create_password_reset_code(array<char, PASSWORD_RESET_CODE_LENGTH*2> & code, ID user_id, string const& email)
{
	uint8_t bytes[PASSWORD_RESET_CODE_LENGTH];

	for(int attempts = 0; attempts < 3; attempts++) {
		try_create_random(bytes, PASSWORD_RESET_CODE_LENGTH);

		try {
			DB::create_password_reset_code(bytes, user_id, email);

			bytes_to_hex_string(ArrayPointer<const uint8_t>(bytes, sizeof bytes), 
				ArrayPointer<char>(code.data(), code.size()));

			return;
		}
		catch (exception const& e){
			spdlog::error(e.what());
		}
	}
	assert_(false);
}

optional<pair<unsigned int, string>> verify_password_reset_code(string const& code)
{
	// TODO: Create a constructor for ArrayPointer<const char> that takes a String
	if(code.length() != Users::PASSWORD_RESET_CODE_LENGTH*2 || 
		!is_hex_string(ArrayPointer<const char>(code.data(), Users::PASSWORD_RESET_CODE_LENGTH))) {
		return nullopt;
	}

	uint8_t bytes[Users::PASSWORD_RESET_CODE_LENGTH];

	hex_string_to_bytes(ArrayPointer<uint8_t>(bytes, sizeof bytes),
		ArrayPointer<const char>(code.data(), code.size()));
	
	return DB::verify_password_reset_code(bytes);
}

optional<Users::ID> reset_password(string const& code, string const& password) 
{
	if(code.length() != Users::PASSWORD_RESET_CODE_LENGTH*2 || 
		!is_hex_string(ArrayPointer<const char>(code.data(), Users::PASSWORD_RESET_CODE_LENGTH))) {
		return false;
	}

	uint8_t bytes[Users::PASSWORD_RESET_CODE_LENGTH];

	hex_string_to_bytes(ArrayPointer<uint8_t>(bytes, sizeof bytes),
		ArrayPointer<const char>(code.data(), code.size()));
	
	return DB::reset_password(bytes, password);
}

}