#include "Random.h"

#define OPENSSL_API_COMPAT 0x10100000L
#include <openssl/rand.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

using namespace std;

void try_create_random(uint8_t * data, uint8_t n) {
	constexpr auto e = "RAND_bytes failure";

	for(int attempts = 0; attempts < 3; attempts++) {
		int success = RAND_bytes(data, n);
		if(success == 1) {
			return;
		}
		else {
			spdlog::error(e);
		}
	}
	throw runtime_error(e);
}