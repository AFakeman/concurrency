headers := $(wildcard *.hpp)
main: main.cpp $(headers)
	g++ -std=c++17 -Wall -Werror -g main.cpp -o main

test: main
	./main
