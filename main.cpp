#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "table.h"

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

int decodeCodePoint(const uint8_t* bytes, int length)
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
		int value = decodeCodePoint(bytes, *length);
		*length = 0;
		printf("U+%04X: %s\n", value, (value >= 0 && value < TABLE_LENGTH) ? g_unicodeTable[value] : MISSING_CODEPOINT_STRING);
	} else if (isMalformedCodePoint(bytes, *length)) {
		printf("Encountered malformed UTF-8 sequence. Aborting.\n");
		exit(1);
	}
	// else assume it's potentially valid, just incomplete.
}

int main(int argc, char** argv)
{
	uint8_t bytes[4];
	int length;
	if (argc <= 1) {
		// read from stdin
		length = 0;
		int c;
		while ((c = getchar()) > 0) {
			bytes[length++] = c;
			tryToPrint(bytes, &length);
		}
	}
	else {
		for (int i = 1; i < argc; ++i) {
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
