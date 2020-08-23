#pragma once

#include <cstdint>

struct address_t {
	uint8_t plane;
	uint8_t table;
	uint8_t index;
};

address_t addressForCodepoint(uint32_t codepoint)
{
	address_t address;
	address.index = codepoint & 0xff;
	address.table = (codepoint >> 8) & 0xff;
	address.plane = (codepoint >> 16) & 0xff;
	return address;
}

uint32_t codepointAtAddress(address_t address)
{
	return (address.plane << 16) | (address.table << 8) | address.index;
}
