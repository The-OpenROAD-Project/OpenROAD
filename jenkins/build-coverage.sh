#!/bin/bash
set -x
set -e
cmake -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON -B build .
time cmake --build build -j 8
