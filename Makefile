all: utf8info

utf8info: main.cpp table.h table.cpp
	clang main.cpp table.cpp -o utf8info -std=c++11 -lstdc++

table.h table.cpp:
	./maketable.sh

clean:
	rm -f utf8info
	rm -f table.h
	rm -f table.cpp
