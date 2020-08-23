all: bin/utf8info

ucd/UnicodeData.txt ucd/extracted/DerivedName.txt update:
	@echo "Downloading latest Unicode source data..."
	@mkdir -p ucd/
	@curl -s -o ucd/UCD.zip https://www.unicode.org/Public/UCD/latest/ucd/UCD.zip
	@curl -s -o ucd/Unihan.zip https://www.unicode.org/Public/UCD/latest/ucd/Unihan.zip
	@unzip ucd/UCD.zip -d ucd/
	@unzip ucd/Unihan.zip -d ucd/

bin/utf8info: main.cpp lookup.h gen/table.h gen/table.cpp
	@echo "Building utf8info..."
	@mkdir -p bin/
	@$(CXX) main.cpp gen/table.cpp -std=c++17 -lstdc++ -o bin/utf8info -Wall -Wextra -Wpedantic

bin/tablegen: lookup.h tablegen.cpp
	@echo "Building table generator..."
	@mkdir -p bin/
	@$(CXX) tablegen.cpp -std=c++17 -lstdc++ -o bin/tablegen -Wall -Wextra -Wpedantic

gen/table.h gen/table.cpp: bin/tablegen ucd/UnicodeData.txt ucd/extracted/DerivedName.txt
	@echo "Building data tables..."
	@mkdir -p gen/
	@./bin/tablegen

install: bin/utf8info
	cp bin/utf8info /usr/local/bin/utf8info

clean:
	rm -rf bin/
	rm -rf gen/
	rm -rf ucd/

.PHONY: all update install clean
