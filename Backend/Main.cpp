#include "Server/Server.h"
#include "DB/DB.h"
#include "Users.h"
#include "Email.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include "Config.h"

#include "AWS.h"

using namespace std;

void run_tests();

int main(int argc, const char ** argv) {

	aws_init();

	#ifdef DEBUG
		spdlog::set_level(spdlog::level::trace);
		run_tests();
	#else
		spdlog::set_level(spdlog::level::info);
	#endif

	if(argc == 2) {
		spdlog::info("Using config file: {}", argv[1]);
		read_config_file(argv[1]);
	}
	else {
		read_config_file(string("config"));
	}

	Emails::start_thread();

	// start_sync_threads();

	// cout << hash_time(1) << '\n';
	WebServer::start_server();

	Emails::stop_thread();

	aws_deinit();
	return 0;
}

#ifdef DEBUG
void TEST__hex_string_to_bytes();
void TEST__bytes_to_hex_string();
void TEST__ip_from_string();
void run_tests() {
	TEST__hex_string_to_bytes();
	TEST__bytes_to_hex_string();
	TEST__ip_from_string();
}
#endif