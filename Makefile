all: utf8info

utf8info: main.cpp table.h table.cpp
	$(CXX) main.cpp table.cpp -std=c++0x -lstdc++ -o utf8info

tablegen: tablegen.cpp
	$(CXX) tablegen.cpp -std=c++0x -lstdc++ -o tablegen

table.h table.cpp: tablegen
	./tablegen

install: utf8info
	cp utf8info /usr/local/bin/utf8info

clean:
	rm -f utf8info
	rm -f tablegen
	rm -f table.h
	rm -f table.cpp
