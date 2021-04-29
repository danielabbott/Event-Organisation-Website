#pragma once

#include <cstdint>
#include <string>

struct IPv6Address {
	union {
		uint8_t bytes[16];

		uint16_t words[8];
	};

	IPv6Address() {
		for(int i = 0; i < 15; i++) bytes[i] = 0;
		bytes[15] = 1;
	}

	IPv6Address(std::string);

	std::string to_string();

	// ipv4 address stored in words[6] and words [7]
	bool is_ipv4() {
		if(words[0] == 0 && words[1] == 0 && words[2] == 0 && words[3] == 0 && words[4] == 0) {
			if(words[5] == 0 || words[5] == 0xffff) {
				return true;
			}
		}
		return false;
	}
};
