#!/bin/bash

echo "build target: $1"

if [[ $1 = "simTask" ]]; then
    echo "make simTask => out/$1"
    mkdir -p out
    shred -s 512K - > out/simTask
    echo "simTask generated: $(ls out/$1 -al -h)"
elif [[ $1 = "ns3" ]]; then
    case="my-ndn-simple2"
    echo "running ns3 simulation for case: $case"
    shred -s 512K - > out/Manager0.simTask.out
    cd ..
    mkdir -p my-ndn-simple2-log
    NS_LOG=$case ../../../waf --run=$case --vis
    echo "all log files saved: $(ls $case-log -al -h)"
else
    echo "make $1.cpp => out/$1"
    mkdir -p out/log
    g++ --std=c++0x -I/usr/local/include/ndn-cxx $1.cpp -Dcflag_is_simulation=0 $(pkg-config --cflags --libs libndn-cxx) -o out/$1
    echo "output executables: $(ls out/$1 -al -h)"
fi;
