#!/bin/bash
# jenkins does not source bashrc
source ~/.bashrc
scl enable llvm-toolset-7.0 bash
export CC=/opt/rh/llvm-toolset-7.0/root/usr/bin/clang
export CXX=/opt/rh/llvm-toolset-7.0/root/usr/bin/clang
mkdir build_clang
cd build_clang
cmake ..
time make -j 8
