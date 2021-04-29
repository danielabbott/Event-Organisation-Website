#include "IP.h"
#include <sstream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <iostream>
#include <cstring>
#include "Assert.h"

using namespace std;

unsigned short swap_endianness(unsigned short x) {
	return ((x & 0xff) << 8) | (x >> 8);
}

string IPv6Address::to_string() {
	stringstream ss;

	ss << std::hex << "[";
	int compact0 = -1;
	bool hadFirstDigit = false;
	for (int i = 0; i < 8; i++) {
		unsigned x = swap_endianness(words[i]);

		if (x == 0) {
			if (compact0 == -1 && i < 7) {
				compact0 = 1;
				continue;
			}
			else if (compact0 != -999 && i < 7) {
				compact0++;
				continue;
			}
			else if(compact0 > 0 && i == 7) {
				ss << ":";
				break;
			}
		}
		else {
			if (compact0 == 1) {
				ss << "0:";
				compact0 = -999;
				hadFirstDigit = true;
			}
			else if (compact0 > 1) {
				if (compact0 == 7 || !hadFirstDigit) {
					ss << ":";

				}
				ss << ":";
				compact0 = -999;
			}
		}
		ss << x;
		hadFirstDigit = true;

		if (i != 7) {
			ss << ":";
		}
	}
	ss << ']';
	return ss.str();
}

static unsigned long to_int(string const& s, size_t * idx, unsigned int base) {
	const char * s2 = &s.c_str()[*idx];
	char * end = nullptr;
	auto x = strtoul (s2, &end, base);
	*idx += end - s2;
	return x;
}

IPv6Address::IPv6Address(string s)
{
	bool ipv4 = s.find('.') != string::npos;
	if(ipv4 && s.find(':') != string::npos) {
		throw runtime_error("Invalid IP address (contains both '.' and ':'");
	}

	if(ipv4) {
		unsigned char octet_values[4];

		size_t idx = 0;
		unsigned int octet = 0;
		while(octet < 4) {
			octet_values[octet] = to_int(s, &idx, 10);
			if(idx >= s.size() && octet < 3) throw runtime_error("1");
			if(octet < 3 && s[idx] != '.') throw runtime_error("2");
			idx++;
			octet++;
		}

		words[0] = words[1] = words[2] = words[3] = words[4] = words[5] = 0;
		bytes[12] = octet_values[1];
		bytes[13] = octet_values[0];
		bytes[14] = octet_values[3];
		bytes[15] = octet_values[2];
	}
	else {
		size_t idx = 0;

		vector<optional<unsigned short>> digit_groups;

		if(s[0] == ':') idx=1;

		while(idx < s.size()) {
			if(s[idx] == ':') {
				if(idx+1 >= s.size()) break;
				idx++;

				digit_groups.push_back(nullopt);
			}
			else {
				digit_groups.push_back(to_int(s, &idx, 16));
				idx++;
			}
		}

		unsigned int dst_i = 0;
		unsigned int src_i = 0;
		while(dst_i < 8 && src_i < digit_groups.size()) {
			if(!digit_groups[src_i].has_value()) {
				// Fill with 0
				unsigned zeroes_to_add = 8 - (digit_groups.size() - 1);
				for(unsigned int i = 0; i < zeroes_to_add; i++) {
					words[dst_i++] = 0;
				}
				src_i++;
			}
			else {
				words[dst_i++] = swap_endianness(digit_groups[src_i++].value());
			}
		}
		while(dst_i < 8) {
			words[dst_i++] = 0;
		}


	}
}

void TEST__ip_from_string()
{
	vector<const char *> tests = {
		"::1",
		"3210::",
		"2001:630:301:5254:ed03:6c60:111c:cd76",
		"2001:630::6c60:111c:cd76",
		// "181.211.111.223",
	};

	for(auto s : tests) {
		IPv6Address ip(s);
		string got;
		auto got_ = ip.to_string();
		if(got_[0] == '[') {
			got = got_.substr(1, got_.size()-2);
		}
		else got = move(got_);

		if(strcmp(s, got.c_str())) {
			cout << "Expected: [" << s << "], got: [" << got << "]\n";
			assert_(false);
		}
	}
}
