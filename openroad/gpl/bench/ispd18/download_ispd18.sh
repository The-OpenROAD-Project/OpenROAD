#!/bin/sh
mkdir -p tar
for num in 1 2 3 4 5 6 7 8 9 10 
do
    fname=ispd18_test${num}
    wget http://www.ispd.cc/contests/18/${fname}.tgz
    mkdir -p ${fname}
    tar zxvf ${fname}.tgz -C ./$fname/
  done 

mv *.tar tar/
