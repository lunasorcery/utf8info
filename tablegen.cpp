#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "lookup.h"



int const NUM_VALUES_PER_TABLE = 256;
int const NUM_TABLES_PER_PLANE = 256;
int const NUM_PLANES = 17;

struct codepoint_info_t {
	std::string name;
	std::string definition;
};

struct table_t {
	codepoint_info_t values[NUM_VALUES_PER_TABLE];
	bool used = false;
};

struct plane_t {
	table_t tables[NUM_TABLES_PER_PLANE];
	bool used = false;
};

struct range_t {
	uint32_t start, end;
	std::string name;
};

plane_t g_planes[NUM_PLANES];

std::vector<range_t> g_ranges;



// strtok doesn't handle empty fields
// this variant does
char* betterStrtok(char* str, char const* delimiters)
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
			char* result = internalStr;
			internalStr += i+1;
			return result;
		}
	}
	
	return nullptr;
}

void addCodepointName(uint32_t codepoint, std::string const& name)
{
	address_t const address = addressForCodepoint(codepoint);

	std::string const& currentName = g_planes[address.plane].tables[address.table].values[address.index].name;
	if (currentName.empty())
	{
		g_planes[address.plane].tables[address.table].values[address.index].name = name;
		g_planes[address.plane].tables[address.table].used = true;
		g_planes[address.plane].used = true;
	}
	else if (currentName != name)
	{
		printf("Name Conflict at U+%04X:\n", codepoint);
		printf("Currently [%s]\n", currentName.c_str());
		printf("Replacing with [%s]\n", name.c_str());
	}
}

void addCodepointDefinition(uint32_t codepoint, std::string const& definition)
{
	address_t const address = addressForCodepoint(codepoint);

	std::string const& currentDefinition = g_planes[address.plane].tables[address.table].values[address.index].definition;
	if (currentDefinition.empty())
	{
		g_planes[address.plane].tables[address.table].values[address.index].definition = definition;
		g_planes[address.plane].tables[address.table].used = true;
		g_planes[address.plane].used = true;
	}
	else if (currentDefinition != definition)
	{
		printf("Definition Conflict at U+%04X:\n", codepoint);
		printf("Currently [%s]\n", currentDefinition.c_str());
		printf("Replacing with [%s]\n", definition.c_str());
	}
}

void addRange(uint32_t start, uint32_t end, std::string const& name)
{
	std::string formatName = name;
	if (!formatName.empty() && formatName[formatName.length()-1] == '*')
	{
		formatName = formatName.substr(0,formatName.length()-1) + "%04X";
	}

	range_t range;
	range.start = start;
	range.end = end;
	range.name = formatName;
	g_ranges.push_back(range);
}

void addOfficialUnicodeData()
{
	FILE* fh = fopen("ucd/UnicodeData.txt", "rb");
	if (!fh)
	{
		printf("couldn't open ucd/UnicodeData.txt for reading!\n");
		exit(1);
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
			addCodepointName(codepoint, oldName);
		}
		else if (name[0] == '<')
		{
			// skip ranges
			// most of these are handled in DerivedName anyway
			// and we manually add the others in addPrivateUseRanges()
		}
		else
		{
			addCodepointName(codepoint, name);
		}
	}

	fclose(fh);
}

void addOfficialDerivedNames()
{
	FILE* fh = fopen("ucd/extracted/DerivedName.txt", "rb");
	if (!fh)
	{
		printf("couldn't open ucd/extracted/DerivedName.txt for reading!\n");
		exit(1);
	}

	char lineBuffer[1024];
	while (fgets(lineBuffer, 1024, fh) != nullptr)
	{
		if (lineBuffer[0] == '#' || lineBuffer[0] == '\r' || lineBuffer[0] == '\n')
			continue;

		char* codepointStr = betterStrtok(lineBuffer, ";\n");
		char const* name   = betterStrtok(nullptr,    ";\n");

		// handle left-padding of the name
		while (name[0] == ' ' || name[0] == '\t')
			name++;

		char const* rangeStartStr = strtok(codepointStr, ".");
		char const* rangeEndStr   = strtok(nullptr,      ".");

		if (rangeEndStr)
		{
			uint32_t const rangeStart = strtol(rangeStartStr, nullptr, 16);
			uint32_t const rangeEnd   = strtol(rangeEndStr,   nullptr, 16);

			addRange(rangeStart, rangeEnd, name);
		}
		else
		{
			uint32_t const codepoint = strtol(rangeStartStr, nullptr, 16);
			addCodepointName(codepoint, name);
		}
	}

	fclose(fh);
}

void addUnihanDefinitions()
{
	FILE* fh = fopen("ucd/Unihan_Readings.txt", "rb");
	if (!fh)
	{
		printf("couldn't open ucd/Unihan_Readings.txt for reading!\n");
		exit(1);
	}

	char lineBuffer[1024];
	while (fgets(lineBuffer, 1024, fh) != nullptr)
	{
		if (lineBuffer[0] == '#' || lineBuffer[0] == '\r' || lineBuffer[0] == '\n')
			continue;

		char const* codepointStr = betterStrtok(lineBuffer, "\t\n");
		char const* type         = betterStrtok(nullptr,    "\t\n");
		char const* value        = betterStrtok(nullptr,    "\t\n");

		if (strcmp(type, "kDefinition") == 0 && (codepointStr[0] == 'U') && (codepointStr[1] == '+'))
		{
			uint32_t const codepoint = strtol(codepointStr+2, nullptr, 16);
			addCodepointDefinition(codepoint, value);
		}
	}
}

void addPrivateUseRanges()
{
	// sourced from UnicodeData.txt
	// ideally this should be parsed automatically
	// but I'm being lazy for now

	addRange(0xD800, 0xDB7F, "Non Private Use High Surrogate");
	addRange(0xDB80, 0xDBFF, "Private Use High Surrogate");
	addRange(0xDC00, 0xDFFF, "Low Surrogate");
	addRange(0xE000, 0xF8FF, "Private Use");
	addRange(0xF0000, 0xFFFFD, "Plane 15 Private Use");
	addRange(0x100000, 0x10FFFD, "Plane 16 Private Use");
}

void addCustomOverrides()
{
	addCodepointName(0xF200, "Private Use, Ubuntu logo in some fonts");
	addCodepointName(0xF8FF, "Private Use, Apple logo on macOS/iOS systems");
}

void writeGeneratedHeader()
{
	FILE* fh = fopen("gen/table.h", "w");
	if (!fh)
	{
		printf("couldn't open gen/table.h for writing!\n");
		exit(1);
	}

	fprintf(fh, "#pragma once\n");
	fprintf(fh, "#include <cstdint>\n");
	fprintf(fh, "static int const NUM_PLANES = 0x%X;\n", NUM_PLANES);
	fprintf(fh, "static int const NUM_TABLES_PER_PLANE = 0x%X;\n", NUM_TABLES_PER_PLANE);
	fprintf(fh, "static int const NUM_VALUES_PER_TABLE = 0x%X;\n", NUM_VALUES_PER_TABLE);
	fprintf(fh, "static int const NUM_RANGES = 0x%X;\n", (uint32_t)g_ranges.size());
	fprintf(fh, "struct codepoint_info_t { char const* name; char const* definition; };\n");
	fprintf(fh, "struct range_t { uint32_t start, end; char const* name; };\n");
	fprintf(fh, "extern codepoint_info_t const* const* g_planes[NUM_PLANES];\n");
	fprintf(fh, "extern range_t g_ranges[NUM_RANGES];\n");
	fclose(fh);
}

void writeGeneratedSource()
{
	FILE* fh = fopen("gen/table.cpp", "w");
	if (!fh)
	{
		printf("couldn't open gen/table.cpp for writing!\n");
		exit(1);
	}

	fprintf(fh, "#include \"table.h\"\n");

	for (uint32_t plane = 0; plane < NUM_PLANES; ++plane)
	{
		for (uint32_t table = 0; table < NUM_TABLES_PER_PLANE; ++table)
		{
			if (g_planes[plane].tables[table].used)
			{
				fprintf(fh, "codepoint_info_t g_plane%02X_table%02X[NUM_VALUES_PER_TABLE] = {\n", plane, table);
				for (uint32_t index = 0; index < NUM_VALUES_PER_TABLE; ++index)
				{
					address_t address;
					address.plane = plane;
					address.table = table;
					address.index = index;
					fprintf(fh, "/*U+%04X*/ {", codepointAtAddress(address));

					std::string const& name = g_planes[plane].tables[table].values[index].name;
					if (!name.empty())
					{
						// naively assume that names don't contain characters we need to escape
						fprintf(fh, "\"%s\", ", name.c_str());
					}
					else
					{
						fprintf(fh, "nullptr, ");
					}

					std::string const& definition = g_planes[plane].tables[table].values[index].definition;
					if (!definition.empty())
					{
						// naively assume that names don't contain characters we need to escape
						fprintf(fh, "\"%s\", ", definition.c_str());
					}
					else
					{
						fprintf(fh, "nullptr, ");
					}

					fprintf(fh, "},\n");
				}
				fprintf(fh, "};\n\n");
			}
		}
	}

	for (uint32_t plane = 0; plane < NUM_PLANES; ++plane)
	{
		if (g_planes[plane].used)
		{
			fprintf(fh, "codepoint_info_t const* g_plane%02X[NUM_TABLES_PER_PLANE] = {\n", plane);
			for (uint32_t table = 0; table < NUM_TABLES_PER_PLANE; ++table)
			{
				if (g_planes[plane].tables[table].used)
				{
					fprintf(fh, "g_plane%02X_table%02X,\n", plane, table);
				}
				else
				{
					fprintf(fh, "nullptr,\n");
				}
			}
			fprintf(fh, "};\n\n");
		}
	}

	fprintf(fh, "codepoint_info_t const* const* g_planes[NUM_PLANES] = {\n");
	for (uint32_t plane = 0; plane < NUM_PLANES; ++plane)
	{
		if (g_planes[plane].used)
		{
			fprintf(fh, "g_plane%02X,\n", plane);
		}
		else
		{
			fprintf(fh, "nullptr,\n");
		}
	}
	fprintf(fh, "};\n\n");

	fprintf(fh, "range_t g_ranges[NUM_RANGES] = {\n");
	for (range_t const& range : g_ranges)
	{
		fprintf(fh, "{ 0x%04X, 0x%04X, \"%s\" },\n", range.start, range.end, range.name.c_str());
	}
	fprintf(fh, "};\n\n");

	fclose(fh);
}

int main()
{
	// add values from DerivedName.txt
	addOfficialDerivedNames();

	// DerivedName.txt misses the control characters
	// so source those from the UnicodeData.txt
	addOfficialUnicodeData();

	addUnihanDefinitions();

	addPrivateUseRanges();
	addCustomOverrides();

	writeGeneratedHeader();
	writeGeneratedSource();
	return 0;
}
