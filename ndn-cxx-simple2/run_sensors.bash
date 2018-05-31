#!/bin/bash

for i in {0..49}
do
  id=$(printf "%04d" $i)
  ./camera_main $id &
done

fg &&

