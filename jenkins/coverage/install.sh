#!/bin/bash
set -x
set -e
mkdir -p /OpenROAD/build
cd /OpenROAD
cmake -DCODE_COVERAGE=ON -B build
time cmake --build build -j 8
