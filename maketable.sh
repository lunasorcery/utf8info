#!/bin/bash
rm -f table.h
rm -f table.cpp
echo "const int TABLE_LENGTH = 131072;" >> table.h
echo "extern const char* g_unicodeTable[TABLE_LENGTH];" >> table.h
echo "extern const char* MISSING_CODEPOINT_STRING;" >> table.h
echo "#include \"table.h\"" >> table.cpp
echo "const char* MISSING_CODEPOINT_STRING = \"<missing codepoint>\";" >> table.cpp
echo "const char* g_unicodeTable[TABLE_LENGTH]={" >> table.cpp
for((i=0;i<256;i++)) do
	cut -d ':' -f 2 "unicode-table-data/loc/en/symbols/$(printf '%02X' $i)00.txt" | awk '{$1=$1;printf("\"%s\",\n", $0);}' >> table.cpp
done
for((i=0;i<256;i++)) do
	cut -d ':' -f 2 "unicode-table-data/loc/en/symbols/plane1/1$(printf '%02X' $i)00.txt" | awk '{$1=$1;printf("\"%s\",\n", $0);}' >> table.cpp
done
echo "};" >> table.cpp
