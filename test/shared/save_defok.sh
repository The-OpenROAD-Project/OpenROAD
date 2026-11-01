#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

# Usage: save_defok.sh <test_name>...
# Copies each test's .def from bazel-testlogs to <test_name>.defok.

set -e

script_dir="$(dirname "$(readlink -f "$0")")"
exec "${script_dir}/bazel_save.sh" defok def "$@"
