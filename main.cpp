#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <getopt.h>
#include <string>
#include "gen/table.h"
#include "lookup.h"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

bool g_verbose = false;
bool g_definitions = false;
bool g_printAll = false;

bool isStartingByte(uint8_t b)
{
	// true if matches 0xxxxxxx, 110xxxxx, 1110xxxx, or 11110xxx, false otherwise
	return ((b & 0x80) == 0x00) || ((b & 0xE0) == 0xC0) || ((b & 0xF0) == 0xE0) || ((b & 0xF8) == 0xF0);
}

bool isContinueByte(uint8_t b)
{
	// true if matches 10xxxxxx, false otherwise
	return (b & 0xC0) == 0x80;
}

bool isCompleteValidCodePoint(uint8_t const* bytes, int length)
{
	// all later bytes must be continue bytes
	for (int i = 1; i < length; ++i) {
		if (!isContinueByte(bytes[i])) {
			return false;
		}
	}

	// ensure length matches the bit pattern in the first byte
	switch (length) {
		case 1:
			return ((bytes[0] & 0x80) == 0x00);
		case 2:
			return ((bytes[0] & 0xE0) == 0xC0);
		case 3:
			return ((bytes[0] & 0xF0) == 0xE0);
		case 4:
			return ((bytes[0] & 0xF8) == 0xF0);
		default:
			return false;
	}
}

void printMalformedSequence(uint8_t const* bytes, int length)
{
	eprintf("Encountered malformed byte sequence ");
	for (int i = 0; i < length; ++i) {
		eprintf("%02X ", bytes[i]);
	}
	eprintf("\n");
}

void formatByteAsBits(uint8_t byte, char* bits)
{
	for (int i = 0; i < 8; ++i) {
		bits[i] = ((byte << i) & 0x80) ? '1' : '0';
	}
	bits[8] = '\0';
}

void checkForMalformedSequence(uint8_t const* bytes, int length)
{
	// first byte must be a valid "starting" byte
	if (!isStartingByte(bytes[0])) {
		printMalformedSequence(bytes, length);
		char bits[9];
		formatByteAsBits(bytes[0], bits);
		eprintf("%02X (%s) is not a valid start byte.\n", bytes[0], bits);
		eprintf("Expected one of these bit patterns:\n");
		eprintf("0xxxxxxx, 110xxxxx, 1110xxxx, 11110xxx\n");
		exit(1);
	}

	// subsequent bytes must be "continue" bytes
	for (int i = 1; i < length; ++i) {
		if (!isContinueByte(bytes[i])) {
			printMalformedSequence(bytes, length);
			char bits[9];
			formatByteAsBits(bytes[i], bits);
			eprintf("%02X (%s) is not a valid continue byte.\n", bytes[i], bits);
			eprintf("Expected bit pattern matching 10xxxxxx\n");
			exit(1);
		}
	}
}

uint32_t decodeCodePoint(uint8_t const* bytes, int length)
{
	switch (length) {
		case 1:
			return (bytes[0]&0x7F);
		case 2:
			return (bytes[0]&0x1F)<<6  | (bytes[1]&0x3F);
		case 3:
			return (bytes[0]&0x0F)<<12 | (bytes[1]&0x3F)<<6  | (bytes[2]&0x3F);
		case 4:
			return (bytes[0]&0x07)<<18 | (bytes[1]&0x3F)<<12 | (bytes[2]&0x3F)<<6 | (bytes[3]&0x3F);
		default:
			return 0; // should never happen...
	}
}

int encodeCodepoint(uint32_t codepoint, uint8_t* bytes)
{
	if (codepoint <= 0x7f) {
		bytes[0] = codepoint & 0x7F;
		return 1;
	} else if (codepoint <= 0x7FF) {
		bytes[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
		bytes[1] = 0x80 | (codepoint & 0x3F);
		return 2;
	} else if (codepoint <= 0xFFFF) {
		bytes[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
		bytes[1] = 0x80 | ((codepoint >> 6) & 0x3F);
		bytes[2] = 0x80 | (codepoint & 0x3F);
		return 3;
	} else if (codepoint <= 0x10FFFF) {
		bytes[0] = 0xF0 | ((codepoint >> 18) & 0x07);
		bytes[1] = 0x80 | ((codepoint >> 12) & 0x3F);
		bytes[2] = 0x80 | ((codepoint >> 6) & 0x3F);
		bytes[3] = 0x80 | (codepoint & 0x3F);
		return 4;
	}
	return 0;
}

bool isKnownCodepoint(uint32_t codepoint)
{
	address_t const address = addressForCodepoint(codepoint);
	if (g_planes[address.plane] && g_planes[address.plane][address.table] && g_planes[address.plane][address.table][address.index].name) {
		return true;
	}

	for (uint32_t range = 0; range < NUM_RANGES; ++range) {
		if (codepoint >= g_ranges[range].start && codepoint <= g_ranges[range].end) {
			return true;
		}
	}

	return false;
}

std::string lookupName(uint32_t codepoint)
{
	// try to get an exact match
	address_t const address = addressForCodepoint(codepoint);
	if (g_planes[address.plane] && g_planes[address.plane][address.table] && g_planes[address.plane][address.table][address.index].name)
		return g_planes[address.plane][address.table][address.index].name;

	// else fall back on a patterned range
	for (uint32_t range = 0; range < NUM_RANGES; ++range) {
		if (codepoint >= g_ranges[range].start && codepoint <= g_ranges[range].end) {
			char buffer[1024];
			sprintf(buffer, g_ranges[range].name, codepoint);
			return std::string(buffer);
		}
	}

	// failing that, we don't know what it is
	return "<missing codepoint>";
}

std::string lookupDefinition(uint32_t codepoint)
{
	// try to get an exact match
	address_t const address = addressForCodepoint(codepoint);
	if (g_planes[address.plane] && g_planes[address.plane][address.table] && g_planes[address.plane][address.table][address.index].definition)
		return g_planes[address.plane][address.table][address.index].definition;

	// failing that, we don't know what it is
	return "";
}

void printBytes(uint8_t const* bytes, int length)
{
	for (int i = 0; i < 4; ++i) {
		if (i < length) {
			printf("%02X ", bytes[i]);
		} else {
			printf("   ");
		}
	}
}

void printBytes(uint32_t codepoint)
{
	uint8_t bytes[4];
	int const length = encodeCodepoint(codepoint, bytes);
	printBytes(bytes, length);
}

void printCodepoint(uint32_t codepoint)
{
	std::string const name = lookupName(codepoint);
	std::string const definition = lookupDefinition(codepoint);
	if (g_definitions && !definition.empty())
		printf("U+%04X: %s: %s\n", codepoint, name.c_str(), definition.c_str());
	else
		printf("U+%04X: %s\n", codepoint, name.c_str());
}

void tryToPrint(uint8_t const* bytes, int* length)
{
	if (isCompleteValidCodePoint(bytes, *length)) {
		uint32_t const codepoint = decodeCodePoint(bytes, *length);

		if (g_verbose) {
			printBytes(bytes, *length);
		}
		printCodepoint(codepoint);

		*length = 0;
	} else {
		checkForMalformedSequence(bytes, *length);
	}
	// else assume it's potentially valid, just incomplete.
}

void parseCommandLine(int argc, char** argv)
{
	while (1) {
		static struct option long_options[] = {
			{ "verbose",     no_argument, 0, 'v' },
			{ "definitions", no_argument, 0, 'd' },
			{ "all",         no_argument, 0, 'a' },
			{ 0, 0, 0, 0 }
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "vda", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'v': {
				g_verbose = true;
				break;
			}
			case 'd': {
				g_definitions = true;
				break;
			}
			case 'a': {
				g_printAll = true;
				break;
			}
		}
	}
}

int main(int argc, char** argv)
{
	parseCommandLine(argc, argv);

	if (g_printAll) {
		uint32_t const maxCodepoint = NUM_PLANES * NUM_TABLES_PER_PLANE * NUM_VALUES_PER_TABLE;
		for (uint32_t codepoint = 0; codepoint < maxCodepoint; ++codepoint) {
			if (isKnownCodepoint(codepoint)) {
				if (g_verbose) {
					printBytes(codepoint);
				}
				printCodepoint(codepoint);
			}
		}
		return 0;
	}

	uint8_t bytes[4];
	int length;
	if (optind >= argc) {
		// read from stdin
		length = 0;
		int c;
		while ((c = getchar()) > 0) {
			bytes[length++] = c;
			tryToPrint(bytes, &length);
		}
	}
	else {
		for (int i = optind; i < argc; ++i) {
			length = 0;
			FILE* fh = fopen(argv[i], "rb");
			if (!fh) {
				eprintf("Failed to open file %s\n", argv[i]);
				return 1;
			}
			while (true) {
				fread(&bytes[length], 1, 1, fh);
				if (feof(fh)) {
					break;
				}
				++length;
				tryToPrint(bytes, &length);
			}
			fclose(fh);
		}
	}
	return 0;
}
