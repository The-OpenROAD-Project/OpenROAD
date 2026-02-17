#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

set -e

for test_name in "${@:1}"
do
    if [ -f "results/${test_name}-tcl.def" ]; then
        cp "results/${test_name}-tcl.def" "${test_name}.defok"
        echo "${test_name}"
    elif [ -f "results/${test_name}-py.def" ]; then
        cp "results/${test_name}-py.def" "${test_name}.defok"
        echo "${test_name}"
    else
        echo "\"${test_name}\" def file not found"
    fi
done
