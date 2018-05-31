g++ --std=c++0x -I/usr/local/include/ndn-cxx $1.cpp $(pkg-config --cflags --libs libndn-cxx) -o $1 
