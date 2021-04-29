#pragma once

#include <string>

constexpr const char * PASSWORD_RESET_URL_START = "/?page=password_reset&code=";
constexpr const char * EVENT_URL_START = "?page=event&id=";

void read_config_file(std::string const& config_file_path);

enum class Setting {
	database_name,
	database_username,
	database_password,
	website_url,
	max_accounts_per_ip_per_day,
	email_account,
	email_password
};

std::string const& get_setting(Setting);
