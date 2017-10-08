#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>

static char** tables[0x1000];

void setCodepointName(uint32_t codepoint, const char* name) {
	if (name) {
		char* clone = new char[strlen(name)+1];
		strcpy(clone, name);
		tables[codepoint >> 8][codepoint & 0xff] = clone;
	} else {
		tables[codepoint >> 8][codepoint & 0xff] = nullptr;
	}
}

void applyCustomOverrides() {
	setCodepointName(0xF200, "Private Use, Ubuntu logo in some fonts");
	setCodepointName(0xF8FF, "Private Use, Apple logo on macOS/iOS systems");
}

int main() {
	for (int plane = 0; plane < 0x10; ++plane) {
		for (int subplane = 0; subplane < 0x100; ++subplane) {
			int tableIndex = plane << 8 | subplane;

			char filepath[256];
			if (plane == 0) {
				sprintf(filepath, "unicode-table-data/loc/en/symbols/%02X00.txt", subplane);
			} else {
				sprintf(filepath, "unicode-table-data/loc/en/symbols/plane%X/%X%02X00.txt", plane, plane, subplane);
			}

			FILE* fh = fopen(filepath, "rb");
			if (fh) {
				if (tables[tableIndex] == nullptr) {
					tables[tableIndex] = new char*[0x100];
				}
				char lineBuffer[1024];
				while (fgets(lineBuffer, 1024, fh) != nullptr) {
					char* end;
					uint32_t codepoint = strtol(lineBuffer, &end, 16);
					if (codepoint >> 8 == tableIndex) { // ignore misplaced data and empty lines
						char* name = strtok(end+2, ":\n");
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
		fprintf(fh, "static const int TABLE_COUNT = 4096;\n");
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
		for (int i = 0; i < 4096; ++i) {
			if (tables[i] != nullptr) {
				fprintf(fh, "const char* table%03X[256] = {\n", i);
				for (int j = 0; j < 256; ++j) {
					if (tables[i][j] != nullptr) {
						fprintf(fh, "\"%s\",\n", tables[i][j]);
					} else {
						fprintf(fh, "0,\n"); //fprintf(fh, "nullptr,\n");
					}
				}
				fprintf(fh, "};\n");
			}
		}
		fprintf(fh, "const char** g_unicodeTables[TABLE_COUNT] = {\n");
		for (int i = 0; i < 4096; ++i) {
			if (tables[i] != nullptr) {
				fprintf(fh, "table%03X,\n", i);
			} else {
				fprintf(fh, "0,\n"); //fprintf(fh, "nullptr,\n");
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
