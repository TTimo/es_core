# FIXME: use cmake, support all platforms. this is temp OSX only (g++ macports)

default: test1 test2
test1: test1.cpp api.h test_shared.h
	g++-mp-4.7 -g -std=c++11 -o test1 test1.cpp

test2: test2.cpp api.h test_shared.h
	g++-mp-4.7 -g -std=c++11 -o test2 -L/opt/local/lib -lboost_system-mt test2.cpp
