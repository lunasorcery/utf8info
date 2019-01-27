all: utf8info

utf8info: main.cpp table.h table.cpp Makefile
	$(CXX) main.cpp table.cpp -std=c++0x -lstdc++ -o utf8info -Wall -Wextra -Wpedantic

tablegen: tablegen.cpp Makefile
	$(CXX) tablegen.cpp -std=c++0x -lstdc++ -o tablegen -Wall -Wextra -Wpedantic

table.h table.cpp: tablegen Makefile
	./tablegen

install: utf8info Makefile
	cp utf8info /usr/local/bin/utf8info

clean:
	rm -f utf8info
	rm -f tablegen
	rm -f table.h
	rm -f table.cpp
