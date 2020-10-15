#!/bin/bash
set -x
set -e
cmake -B build .
cmake --build build -j 8
