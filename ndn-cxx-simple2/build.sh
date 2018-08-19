#!/bin/bash

echo "build target: $1"

if [[ $1 = "simTask" ]]; then
    echo "make simTask => out/$1"
    mkdir -p out
    shred -s 512K - > out/simTask
    echo "simTask generated: $(ls out/$1 -al -h)"
elif [[ $1 = "ns3" ]]; then
    echo "running ns3 simulation for case: $2"
    cd ..
    mkdir -p $2-log
    NS_LOG=$2 ../../../waf --run=$2 --vis
    echo "all log files saved: $(ls $2-log -al -h)"
else
    echo "make $1.cpp => out/$1"
    mkdir -p out
    g++ --std=c++0x -I/usr/local/include/ndn-cxx $1.cpp -Dcflag_is_simulation=0 $(pkg-config --cflags --libs libndn-cxx) -o out/$1
    echo "output executables: $(ls out/$1 -al -h)"
fi;
