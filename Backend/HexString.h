#pragma once

#include "ArrayPointer.h"
#include <cstdint>

void hex_string_to_bytes(ArrayPointer<uint8_t> bytes, ArrayPointer<const char> str);
void bytes_to_hex_string(ArrayPointer<const uint8_t> bytes, ArrayPointer<char> str);
bool is_hex_string(ArrayPointer<const char> str);