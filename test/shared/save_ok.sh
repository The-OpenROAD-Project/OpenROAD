#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

set -e

script_dir="$(dirname "$(readlink -f "$0")")"
bazel_pending=()

for test_name in "${@:1}"
do
    if [ -f "results/${test_name}-tcl.log" ]; then
        cp "results/${test_name}-tcl.log" "${test_name}.ok"
        echo "${test_name}"
    elif [ -f "results/${test_name}-py.log" ]; then
        cp "results/${test_name}-py.log" "${test_name}.ok"
        echo "${test_name}"
    else
        bazel_pending+=("${test_name}")
    fi
done

if [ ${#bazel_pending[@]} -gt 0 ]; then
    "${script_dir}/bazel_save.sh" ok log "${bazel_pending[@]}"
fi
