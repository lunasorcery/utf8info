#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>

static const int NUM_PLANES = 0x10;
static const int NUM_SUBPLANES = 0x100;
static const int NUM_TABLES = 0x1000;
static const int TABLE_LENGTH = 0x100;

static std::string tables[NUM_TABLES][TABLE_LENGTH];
static bool tablesUsed[NUM_TABLES];
static bool codepointsUsed[NUM_TABLES][TABLE_LENGTH];

void setCodepointName(uint32_t codepoint, const char* name) {
	if (!name)
		return;

	const uint32_t tableIndex = codepoint >> 8;
	const uint32_t tableEntry = codepoint & 0xff;

	std::string trimmedName = std::string(name);
	while (trimmedName[trimmedName.length() - 1] == ' ')
	{
		trimmedName = std::string(
			trimmedName.begin(),
			trimmedName.end() - 1
		);
	}

	tables[tableIndex][tableEntry] = trimmedName;
	tablesUsed[tableIndex] = true;
	codepointsUsed[tableIndex][tableEntry] = true;
}

void applyCustomOverrides() {
	setCodepointName(0xF200, "Private Use, Ubuntu logo in some fonts");
	setCodepointName(0xF8FF, "Private Use, Apple logo on macOS/iOS systems");
}

int main() {
	for (uint32_t i = 0; i < NUM_TABLES; ++i) {
		tablesUsed[i] = false;
		for (uint32_t j = 0; j < TABLE_LENGTH; ++j) {
			codepointsUsed[i][j] = false;
		}
	}

	for (uint32_t plane = 0; plane < NUM_PLANES; ++plane) {
		for (uint32_t subplane = 0; subplane < NUM_SUBPLANES; ++subplane) {
			const uint32_t tableIndex = plane << 8 | subplane;

			char filepath[256];
			if (plane == 0) {
				sprintf(filepath, "unicode-table-data/loc/en/symbols/%02X00.txt", subplane);
			} else {
				sprintf(filepath, "unicode-table-data/loc/en/symbols/plane%X/%X%02X00.txt", plane, plane, subplane);
			}

			FILE* fh = fopen(filepath, "rb");
			if (fh) {
				char lineBuffer[1024];
				while (fgets(lineBuffer, 1024, fh) != nullptr) {
					char* end;
					uint32_t codepoint = strtol(lineBuffer, &end, 16);
					if (codepoint >> 8 == tableIndex) { // ignore misplaced data and empty lines
						const char* name = strtok(end+2, ":\n");
						setCodepointName(codepoint, name);
					}
				}
				fclose(fh);
			}
		}
	}

	applyCustomOverrides();

	FILE* fh;

	fh = fopen("table.h", "w");
	if (fh) {
		fprintf(fh, "#pragma once\n");
		fprintf(fh, "static const int TABLE_COUNT = 0x%X;\n", NUM_TABLES);
		fprintf(fh, "extern const char** g_unicodeTables[TABLE_COUNT];\n");
		fprintf(fh, "extern const char* MISSING_CODEPOINT_STRING;\n");
		fclose(fh);
	} else {
		printf("couldn't open table.h for writing!\n");
		return 1;
	}

	fh = fopen("table.cpp", "w");
	if (fh) {
		fprintf(fh, "#include \"table.h\"\n");
		for (uint32_t i = 0; i < NUM_TABLES; ++i) {
			if (tablesUsed[i]) {
				fprintf(fh, "const char* table%03X[0x%X] = {\n", i, TABLE_LENGTH);
				for (uint32_t j = 0; j < TABLE_LENGTH; ++j) {
					if (codepointsUsed[i][j]) {
						fprintf(fh, "\"%s\",\n", tables[i][j].c_str());
					} else {
						fprintf(fh, "nullptr,\n");
					}
				}
				fprintf(fh, "};\n");
			}
		}
		fprintf(fh, "const char** g_unicodeTables[TABLE_COUNT] = {\n");
		for (uint32_t i = 0; i < NUM_TABLES; ++i) {
			if (tablesUsed[i]) {
				fprintf(fh, "table%03X,\n", i);
			} else {
				fprintf(fh, "nullptr,\n");
			}
		}
		fprintf(fh, "};\n");
		fprintf(fh, "const char* MISSING_CODEPOINT_STRING = \"<missing codepoint>\";\n");
		fclose(fh);
	} else {
		printf("couldn't open table.cpp for writing!\n");
		return 1;
	}
}
