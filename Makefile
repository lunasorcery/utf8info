FUZZ = afl-cc
all: bin/utf8info

ucd/UnicodeData.txt ucd/extracted/DerivedName.txt ucd/Unihan_Readings.txt update:
	@echo "Downloading latest Unicode source data..."
	@mkdir -p ucd/
	@curl -s -o ucd/UCD.zip https://www.unicode.org/Public/UCD/latest/ucd/UCD.zip
	@curl -s -o ucd/Unihan.zip https://www.unicode.org/Public/UCD/latest/ucd/Unihan.zip
	@unzip -o ucd/UCD.zip    UnicodeData.txt           -d ucd/
	@unzip -o ucd/UCD.zip    extracted/DerivedName.txt -d ucd/
	@unzip -o ucd/Unihan.zip Unihan_Readings.txt       -d ucd/
	@touch ucd/UnicodeData.txt
	@touch ucd/extracted/DerivedName.txt
	@touch ucd/Unihan_Readings.txt
	@rm -f ucd/UCD.zip
	@rm -f ucd/Unihan.zip

bin/utf8info: main.cpp lookup.h gen/table.h gen/table.cpp
	@echo "Building utf8info..."
	@mkdir -p bin/
	@$(CXX) main.cpp gen/table.cpp -std=c++17 -lstdc++ -o bin/utf8info -Wall -Wextra -Wpedantic

bin/tablegen: lookup.h tablegen.cpp
	@echo "Building table generator..."
	@mkdir -p bin/
	@$(CXX) tablegen.cpp -std=c++17 -lstdc++ -o bin/tablegen -Wall -Wextra -Wpedantic

gen/table.h gen/table.cpp: bin/tablegen ucd/UnicodeData.txt ucd/extracted/DerivedName.txt ucd/Unihan_Readings.txt
	@echo "Building data tables..."
	@mkdir -p gen/
	@./bin/tablegen

install: bin/utf8info
	cp bin/utf8info /usr/local/bin/utf8info

aflpp: ucd/UnicodeData.txt
	@echo "Building table generator..."
	@mkdir -p bin/
	$(FUZZ) tablegen.cpp -std=c++17 -lstdc++ -o bin/tablegen -Wall -Wextra -Wpedantic
	@echo "Building data tables..."
	@mkdir -p gen/
	@./bin/tablegen
	@echo "Building utf8info..."
	$(FUZZ) main.cpp gen/table.cpp -std=c++17 -lstdc++ -o bin/utf8info -Wall -Wextra -Wpedantic

clean:
	rm -rf bin/
	rm -rf gen/
	rm -rf ucd/

.PHONY: all update install clean aflpp
