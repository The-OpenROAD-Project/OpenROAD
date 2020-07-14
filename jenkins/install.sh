#!/bin/bash
# jenkins does not source bashrc
source ~/.bashrc
mkdir build
cd build
cmake ..
time make -j
