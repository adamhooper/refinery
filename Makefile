CXXFLAGS=-Iinclude -g -Wall -fPIC
CXX=g++

all: obj/librefinery.so

test: test/gtest_main
	LD_LIBRARY_PATH=obj test/gtest_main

test/gtest_main: test/*.cc include/refinery/*.h obj/librefinery.so
	${CXX} ${CXXFLAGS} test/*.cc -lpthread -lgtest -Lobj -lrefinery -o test/gtest_main

obj/librefinery.so: obj/image.o obj/input.o obj/interpolate.o obj/output.o obj/unpack.o
	${CXX} -shared $^ -o $@

obj/image.o: src/image.cc include/refinery/image.h
	${CXX} -c ${CXXFLAGS} $< -o $@

obj/input.o: src/input.cc include/refinery/input.h
	${CXX} -c ${CXXFLAGS} $< -o $@

obj/interpolate.o: src/interpolate.cc include/refinery/interpolate.h include/refinery/image.h
	${CXX} -c ${CXXFLAGS} $< -o $@

obj/output.o: src/output.cc include/refinery/output.h include/refinery/image.h
	${CXX} -c ${CXXFLAGS} $< -o $@

obj/unpack.o: src/unpack.cc include/refinery/unpack.h include/refinery/input.h include/refinery/image.h
	${CXX} -c ${CXXFLAGS} $< -o $@

clean:
	rm -f obj/*.o obj/*.so test/gtest_main
