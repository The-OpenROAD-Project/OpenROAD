#!/bin/bash
set -x
set -e
mkdir -p /OpenROAD/build
cd /OpenROAD/build
cmake ..
make -j 4
