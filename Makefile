#PARAMS=-g -Wall -static-libgcc --target=x86_64-w64-mingw -std=c++2a
PARAMS=-g -Wall -static-libgcc -std=gnu++2a
all: json.cpp json.hpp test.cpp
	clang++-10 json.cpp test.cpp -o test.out ${PARAMS}