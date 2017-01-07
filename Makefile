all: utf8info

utf8info: main.cpp table.h table.cpp
	clang main.cpp table.cpp -o utf8info -std=c++11 -lstdc++

table.h table.cpp:
	curl -s 'https://unicode-table.com/en/a-desc/' -XPOST -H 'Content-Type: application/x-www-form-urlencoded; charset=UTF-8' -H 'Referer: https://unicode-table.com/en/' -H 'Accept: application/json, text/javascript, */*; q=0.01' -H 'Origin: https://unicode-table.com' -H 'X-Requested-With: XMLHttpRequest' --data 'block=0&size=131072&c=0' > table.json
	node generate-table.js
	rm table.json

clean:
	rm -f utf8info
	rm -f table.h
	rm -f table.cpp
