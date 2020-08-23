all: bin/utf8info

ucd/UnicodeData.txt ucd/DerivedName.txt update:
	mkdir -p ucd/
	curl -s  -o ucd/UnicodeData.txt https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt
	curl -s  -o ucd/DerivedName.txt https://www.unicode.org/Public/UCD/latest/ucd/extracted/DerivedName.txt

bin/utf8info: main.cpp lookup.h gen/table.h gen/table.cpp Makefile
	mkdir -p bin/
	$(CXX) main.cpp gen/table.cpp -std=c++17 -lstdc++ -o bin/utf8info -Wall -Wextra -Wpedantic

bin/tablegen: lookup.h tablegen.cpp Makefile
	mkdir -p bin/
	$(CXX) tablegen.cpp -std=c++17 -lstdc++ -o bin/tablegen -Wall -Wextra -Wpedantic

gen/table.h gen/table.cpp: bin/tablegen ucd/UnicodeData.txt ucd/DerivedName.txt Makefile
	mkdir -p gen/
	./bin/tablegen

install: bin/utf8info Makefile
	cp bin/utf8info /usr/local/bin/utf8info

clean:
	rm -rf bin/
	rm -rf gen/
	rm -rf ucd/
