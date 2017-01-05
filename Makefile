all: utf8info

utf8info: main.cpp
	clang main.cpp -o utf8info -std=c++11 -lstdc++

clean:
	rm utf8info
