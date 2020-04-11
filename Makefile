#PARAMS=-g -Wall -static-libgcc --target=x86_64-w64-mingw -std=c++2a
PARAMS=-g -Wall -static-libgcc -std=gnu++2a
all: json.hpp test.cpp
	clang++-10  test.cpp -o test.out ${PARAMS}