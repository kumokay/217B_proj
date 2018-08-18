#!/bin/bash

mkdir -p out

echo "build target: $1"

if [[ $1 = "simTask" ]]; then
    echo "make simTask => out/$1"
    shred -s 512K - > out/simTask
else
    echo "make $1.cpp => out/$1"
    g++ --std=c++0x -I/usr/local/include/ndn-cxx $1.cpp -Dcflag_is_simulation=0 $(pkg-config --cflags --libs libndn-cxx) -o out/$1
fi;

ls out/$1 -al -h
