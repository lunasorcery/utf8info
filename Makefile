all: utf8info

utf8info: main.cpp table.h table.cpp
	$(CXX) main.cpp table.cpp -std=c++0x -lstdc++ -o utf8info

table.h table.cpp:
	./maketable.sh

clean:
	rm -f utf8info
	rm -f table.h
	rm -f table.cpp
