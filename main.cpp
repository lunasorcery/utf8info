#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <getopt.h>
#include "table.h"

bool g_verbose = false;

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

bool isCompleteValidCodePoint(const uint8_t* bytes, int length)
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

bool isMalformedCodePoint(const uint8_t* bytes, int length)
{
	// first byte must be a valid "starting" byte
	if (!isStartingByte(bytes[0])) {
		return true;
	}

	// subsequent bytes must be "continue" bytes
	for (int i = 1; i < length; ++i) {
		if (!isContinueByte(bytes[i])) {
			return true;
		}
	}

	return false;
}

uint32_t decodeCodePoint(const uint8_t* bytes, int length)
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

void tryToPrint(const uint8_t* bytes, int* length)
{
	if (isCompleteValidCodePoint(bytes, *length)) {
		uint32_t value = decodeCodePoint(bytes, *length);

		const char* name = nullptr;
		uint32_t tableIndex = value >> 8;
		if (tableIndex < TABLE_COUNT) {
			if (g_unicodeTables[tableIndex] != nullptr) {
				name = g_unicodeTables[tableIndex][value & 0xff];
			}
		}
		if (name == nullptr) {
			name = MISSING_CODEPOINT_STRING;
		}
		if (g_verbose) {
			for (int i = 0; i < 4; ++i) {
				if (i < *length) {
					printf("%02X ", bytes[i]);
				} else {
					printf("   ");
				}
			}
		}
		printf("U+%04X: %s\n", value, name);

		*length = 0;
	} else if (isMalformedCodePoint(bytes, *length)) {
		printf("Encountered malformed UTF-8 sequence. Aborting.\n");
		exit(1);
	}
	// else assume it's potentially valid, just incomplete.
}

void parseCommandLine(int argc, char** argv)
{
	while (1) {
		static struct option long_options[] = {
			{ "verbose", no_argument, 0, 'v' },
			{ 0, 0, 0, 0 }
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "v", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'v': {
				g_verbose = true;
				break;
			}
		}
	}
}

int main(int argc, char** argv)
{
	parseCommandLine(argc, argv);

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
				printf("Failed to open file %s\n", argv[i]);
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
