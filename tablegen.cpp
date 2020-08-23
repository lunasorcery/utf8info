#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>

static const int TABLE_COUNT = 0x1100;
static const int TABLE_LENGTH = 0x100;

static std::string tables[TABLE_COUNT][TABLE_LENGTH];
static bool tablesUsed[TABLE_COUNT];
static bool codepointsUsed[TABLE_COUNT][TABLE_LENGTH];

// strtok doesn't handle empty fields
// this variant does
char const* betterStrtok(char* str, char const* delimiters)
{
	static char* internalStr = nullptr;

	if (str)
		internalStr = str;

	if (!internalStr)
		return nullptr;

	for (int i = 0; ; ++i)
	{
		if (strchr(delimiters, internalStr[i]))
		{
			internalStr[i] = '\0';
			char const* result = internalStr;
			internalStr += i+1;
			return result;
		}
	}
	
	return nullptr;
}

void setCodepointName(uint32_t codepoint, char const* name)
{
	if (!name)
		return;

	const uint32_t tableIndex = codepoint / TABLE_LENGTH;
	const uint32_t tableEntry = codepoint % TABLE_LENGTH;

	tables[tableIndex][tableEntry] = name;
	tablesUsed[tableIndex] = true;
	codepointsUsed[tableIndex][tableEntry] = true;
}

void addRange(uint32_t start, uint32_t end, char const* format)
{
	for (uint32_t i = start; i <= end; ++i)
	{
		char buffer[1024];
		sprintf(buffer, format, i);
		setCodepointName(i, buffer);
	}
}

void addPatternRanges()
{
	addRange( 0x3400,  0x4DBF, "CJK UNIFIED IDEOGRAPH-%04X");
	addRange( 0x4E00,  0x9FFC, "CJK UNIFIED IDEOGRAPH-%04X");
	addRange( 0xF900,  0xFA6D, "CJK COMPATIBILITY IDEOGRAPH-%04X");
	addRange( 0xFA70,  0xFAD9, "CJK COMPATIBILITY IDEOGRAPH-%04X");
	addRange(0x17000, 0x187F7, "TANGUT IDEOGRAPH-%04X");
	addRange(0x18B00, 0x18CD5, "KHITAN SMALL SCRIPT CHARACTER-%04X");
	addRange(0x18D00, 0x18D08, "TANGUT IDEOGRAPH-%04X");
	addRange(0x1B170, 0x1B2FB, "NUSHU CHARACTER-%04X");
	addRange(0x20000, 0x2A6DD, "CJK UNIFIED IDEOGRAPH-%04X");
	addRange(0x2A700, 0x2B734, "CJK UNIFIED IDEOGRAPH-%04X");
	addRange(0x2B740, 0x2B81D, "CJK UNIFIED IDEOGRAPH-%04X");
	addRange(0x2B820, 0x2CEA1, "CJK UNIFIED IDEOGRAPH-%04X");
	addRange(0x2CEB0, 0x2EBE0, "CJK UNIFIED IDEOGRAPH-%04X");
	addRange(0x2F800, 0x2FA1D, "CJK COMPATIBILITY IDEOGRAPH-%04X");
	addRange(0x30000, 0x3134A, "CJK UNIFIED IDEOGRAPH-%04X");
}

void addCustomOverrides()
{
	setCodepointName(0xF200, "Private Use, Ubuntu logo in some fonts");
	setCodepointName(0xF8FF, "Private Use, Apple logo on macOS/iOS systems");
}

int main()
{
	for (uint32_t i = 0; i < TABLE_COUNT; ++i)
	{
		tablesUsed[i] = false;
		for (uint32_t j = 0; j < TABLE_LENGTH; ++j)
		{
			codepointsUsed[i][j] = false;
		}
	}

	// parse official Unicode data
	{
		FILE* fh = fopen("ucd/UnicodeData.txt", "rb");
		if (!fh)
		{
			printf("couldn't open gen/table.h for writing!\n");
			return 1;
		}

		char lineBuffer[1024];
		while (fgets(lineBuffer, 1024, fh) != nullptr)
		{
			                 char const* codepointStr = betterStrtok(lineBuffer, ";\n");
			                 char const* name         = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* category     = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* u4           = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* u5           = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* u6           = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* u7           = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* u8           = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* u9           = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* u10          = betterStrtok(nullptr,    ";\n");
			                 char const* oldName      = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* u12          = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* upperCase    = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* lowerCase    = betterStrtok(nullptr,    ";\n");
			[[maybe_unused]] char const* titleCase    = betterStrtok(nullptr,    ";\n");
				
			uint32_t codepoint = strtol(codepointStr, nullptr, 16);
			if (strcmp(name, "<control>") == 0)
			{
				// use the alt name for control characters
				setCodepointName(codepoint, oldName);
			}
			else if (name[0] == '<')
			{
				// skip ranges, we'll cheat and handle these separately
			}
			else
			{
				setCodepointName(codepoint, name);
			}
		}
		fclose(fh);
	}

	addPatternRanges();
	addCustomOverrides();

	// create gen/table.h
	{
		FILE* fh = fopen("gen/table.h", "w");
		if (!fh)
		{
			printf("couldn't open gen/table.h for writing!\n");
			return 1;
		}

		fprintf(fh, "#pragma once\n");
		fprintf(fh, "static int const TABLE_COUNT = 0x%X;\n", TABLE_COUNT);
		fprintf(fh, "static int const TABLE_LENGTH = 0x%X;\n", TABLE_LENGTH);
		fprintf(fh, "extern char const* const* g_unicodeTables[TABLE_COUNT];\n");
		fprintf(fh, "extern char const* MISSING_CODEPOINT_STRING;\n");
		fclose(fh);
	}

	// create gen/table.cpp
	{
		FILE* fh = fopen("gen/table.cpp", "w");
		if (!fh)
		{
			printf("couldn't open gen/table.cpp for writing!\n");
			return 1;
		}

		fprintf(fh, "#include \"table.h\"\n");
		for (uint32_t i = 0; i < TABLE_COUNT; ++i)
		{
			if (tablesUsed[i])
			{
				fprintf(fh, "char const* table%03X[0x%X] = {\n", i, TABLE_LENGTH);
				for (uint32_t j = 0; j < TABLE_LENGTH; ++j)
				{
					fprintf(fh, "/*U+%04X*/", i*TABLE_LENGTH+j);
					if (codepointsUsed[i][j])
					{
						fprintf(fh, "\"%s\",\n", tables[i][j].c_str());
					}
					else
					{
						fprintf(fh, "nullptr,\n");
					}
				}
				fprintf(fh, "};\n");
			}
		}
		fprintf(fh, "char const* const* g_unicodeTables[TABLE_COUNT] = {\n");
		for (uint32_t i = 0; i < TABLE_COUNT; ++i)
		{
			if (tablesUsed[i])
			{
				fprintf(fh, "table%03X,\n", i);
			}
			else
			{
				fprintf(fh, "nullptr,\n");
			}
		}
		fprintf(fh, "};\n");
		fprintf(fh, "char const* MISSING_CODEPOINT_STRING = \"<missing codepoint>\";\n");
		fclose(fh);
	}
}
