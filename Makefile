CXXFLAGS=-Iinclude -g -Wall -fPIC
CXX=g++

all: obj/librefinery.so

test: obj/librefinery.so test/gtest_main
	LD_LIBRARY_PATH=obj test/gtest_main

test/gtest_main: test/*.cc
	${CXX} ${CXXFLAGS} test/*.cc -lpthread -lgtest -Lobj -lrefinery -o test/gtest_main

obj/librefinery.so: obj/input.o
	ld -shared $^ -o $@

obj/input.o: src/input.cc include/refinery/input.h
	${CXX} -c ${CXXFLAGS} $< -o $@

clean:
	rm -f obj/*.o obj/*.so test/gtest_main
