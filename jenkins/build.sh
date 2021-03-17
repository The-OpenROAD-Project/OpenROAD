#!/bin/bash
set -x
set -e

if [[ "$#" -gt 0 ]]; then
    case "$@" in
        "coverge" )
            CMAKE_OPTIONS="-DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON"
            ;;
        "no-gui" )
            CMAKE_OPTIONS="-DBUILD_GUI=OFF"
            ;;
        "clang" )
            COMPILER="clang"
            ;;
        *)
            echo "Option not recognized"
            echo "usage: $0"
            echo "       $0 [clang]"
            echo "       $0 [coverage]"
            echo "       $0 [no-gui]"
            return 1
            ;;
    esac
fi

if [[ "$COMPILER" == "clang" ]]; then
    source /opt/rh/llvm-toolset-7.0/enable
    export CC=/opt/rh/llvm-toolset-7.0/root/usr/bin/clang
    export CXX=/opt/rh/llvm-toolset-7.0/root/usr/bin/clang
else
    source /opt/rh/devtoolset-8/enable
fi

cmake "$CMAKE_OPTIONS" -B build .
time cmake --build build -j 8
