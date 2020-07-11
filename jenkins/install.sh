#!/bin/bash
set -x
set -e
cd OpenROAD
mkdir build
cd build
cmake ..
time make -j 8
