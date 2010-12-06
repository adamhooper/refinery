CXXFLAGS=-Iinclude -g -Wall -fPIC
CXX=g++

all: obj/librefinery.so

test: obj/librefinery.so test/gtest_main
	LD_LIBRARY_PATH=obj test/gtest_main

test/gtest_main: test/*.cc include/refinery/*.h
	${CXX} ${CXXFLAGS} test/*.cc -lpthread -lgtest -Lobj -lrefinery -o test/gtest_main

obj/librefinery.so: obj/image.o obj/input.o obj/unpack.o obj/interpolate.o
	${CXX} -shared $^ -o $@

obj/image.o: src/image.cc include/refinery/image.h
	${CXX} -c ${CXXFLAGS} $< -o $@

obj/input.o: src/input.cc include/refinery/input.h
	${CXX} -c ${CXXFLAGS} $< -o $@

obj/interpolate.o: src/interpolate.cc include/refinery/interpolate.h include/refinery/image.h
	${CXX} -c ${CXXFLAGS} $< -o $@

obj/unpack.o: src/unpack.cc include/refinery/unpack.h include/refinery/input.h include/refinery/image.h
	${CXX} -c ${CXXFLAGS} $< -o $@

clean:
	rm -f obj/*.o obj/*.so test/gtest_main
