#pragma once

#include <cstdint>
#include <string>
#include "../HexString.h"
#include <stdexcept>

struct MediaID {
	uint8_t data[1+16];


	std::string to_string() const {
		char s[1+1+32];
		s[0] = data[0];
		s[1] = '/';

		bytes_to_hex_string(ArrayPointer<const uint8_t>(&data[1], 16), ArrayPointer<char>(&s[2], 32));
		return std::string(s, 1+1+32);
	}

	MediaID(){}

	MediaID(std::string const& s) {
		if(s.length() != 34) {
			throw std::runtime_error("Incorrect length");
		}

		if(s[0] != 'a' && s[1] != '/') {
			throw std::runtime_error("Incorrect format (should start with a/)");
		}

		if(!is_hex_string(ArrayPointer<const char>(&s.c_str()[2], 32))) {
			throw std::runtime_error("Incorrect format (not hex string)");
		}

		data[0] = s[0];

		hex_string_to_bytes(ArrayPointer<uint8_t>(&data[1], 16), 
			ArrayPointer<const char>(&s.c_str()[2], 32));
	}
};

struct Media {
	MediaID id;
	bool is_cover_image;
	std::string file_name;
};