#!/bin/bash

set -eo pipefail

cd "$(dirname $(readlink -f $0))/../"

compiler=${1:-gcc}
if [[ "${compiler}" == "gcc" ]]; then
    if [[ -f "/opt/rh/devtoolset-8/enable" ]]; then
        source /opt/rh/devtoolset-8/enable
    fi
    shift 1
fi

if [[ "${compiler}" == "clang" ]]; then
    if [[ -f "/opt/rh/llvm-toolset-7.0/enable" ]]; then
        source /opt/rh/llvm-toolset-7.0/enable
    fi
    shift 1
fi

eval "${@}"
