#include "HexString.h"
#include <cstring>

void hex_string_to_bytes(ArrayPointer<uint8_t> bytes, ArrayPointer<const char> str) {
	assert_(bytes.size()*2 >= str.size());
	assert_(str.size() % 2 == 0);

	for(unsigned int i = 0; i < bytes.size(); i++) {
		unsigned char nibbleHighASCII = str[i*2];
		unsigned char nibbleLowASCII = str[i*2+1];

		constexpr auto from_hex_digit = [](unsigned char x) {
			if(x >= 'a') return 10+(x-'a');
			if(x >= 'A') return 10+(x-'A');
			return x-'0';
		};

		unsigned char nibbleHigh = from_hex_digit(nibbleHighASCII);
		unsigned char nibbleLow = from_hex_digit(nibbleLowASCII);

		bytes[i] = nibbleLow | (nibbleHigh << 4);
	}
}



void bytes_to_hex_string(ArrayPointer<const uint8_t> bytes, ArrayPointer<char> s) {
	assert_(bytes.size()*2 <= s.size());

	for(unsigned int i = 0; i < bytes.size(); i++) {
		unsigned char byte = bytes[i];
		unsigned char nibbleLow = byte & 0xf;
		unsigned char nibbleHigh = byte >> 4;

		constexpr auto to_hex_digit = [](unsigned char x) {
			return x <= 9 ? '0'+x : 'A'+(x-10);
		};

		s[i*2] = to_hex_digit(nibbleHigh);
		s[i*2+1] = to_hex_digit(nibbleLow);
	}
}


bool is_hex_string(ArrayPointer<const char> str) {
	if(str.size() == 0) return false;


	for(unsigned int i = 0; i < str.size(); i++) {
		bool valid = (str[i] >= 'A' && str[i] <= 'F') ||
			(str[i] >= 'a' && str[i] <= 'f') ||
				(str[i] >= '0' && str[i] <= '9');
		if(!valid) {
			return false;
		}
	}
	return true;
}



#ifdef DEBUG
void TEST__hex_string_to_bytes() {
	uint8_t bytes[32];
	memset(bytes, 0, 32);
	bytes[31] = 0x3D;

	char s[64];
	bytes_to_hex_string(ArrayPointer<const uint8_t>(bytes, 32), ArrayPointer<char>(s, 64));

	for(int i = 0; i < 62; i++) {
		assert_(s[i] == '0');
	}

	assert_(s[62] == '3' && s[63] == 'D');
}
void TEST__bytes_to_hex_string() {
	char s[64];
	memset(s, '0', 64);
	s[63] = '1';


	uint8_t bytes[32];

	hex_string_to_bytes(ArrayPointer<uint8_t>(bytes, 32), ArrayPointer<const char>(s, 64));

	for(int i = 0; i < 31; i++) {
		assert_(!bytes[i]);
	}

	assert_(bytes[31] == 1);
}
#endif