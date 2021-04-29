#include "Config.h"
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <cstring>
#include <stdexcept>

using namespace std;

static unordered_map<Setting, string> config;

void read_config_file(string const& config_file_path)
{
	std::ifstream file(config_file_path);
	std::string str; 
	while (std::getline(file, str))
	{
		if(str.rfind("database_name=", 0) == 0) {
			config[Setting::database_name] = str.substr(strlen("database_name="));
		}
		else if(str.rfind("database_username=", 0) == 0) {
			config[Setting::database_username] = str.substr(strlen("database_username="));
		}
		else if(str.rfind("database_password=", 0) == 0) {
			config[Setting::database_password] = str.substr(strlen("database_password="));
		}
		else if(str.rfind("website_url=", 0) == 0) {
			config[Setting::website_url] = str.substr(strlen("website_url="));
		}
		else if(str.rfind("max_accounts_per_ip_per_day=", 0) == 0) {
			config[Setting::max_accounts_per_ip_per_day] = str.substr(strlen("max_accounts_per_ip_per_day="));
		}
		else if(str.rfind("email_account=", 0) == 0) {
			config[Setting::email_account] = str.substr(strlen("email_account="));
		}
		else if(str.rfind("email_password=", 0) == 0) {
			config[Setting::email_password] = str.substr(strlen("email_password="));
		}
	}
}

std::string const& get_setting(Setting s)
{
	if(config.find(s) == config.end()) {
		throw runtime_error("Setting value not found");
	}
	return config[s];
}
