#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

set -e

# Find the module from the current directory under openroad/<module>/.
_get_tool_name()
{
    local script_path
    script_path=$(pwd -P)
    local module_path=${script_path##*/openroad/}
    tool=${module_path%%/*}
}

#
# Usage: ./regression [-R <regex>] [bazel test args...]
# Selects the test targets below //openroad/<tool>/ whose label matches <regex>
# and runs them in one bazel test call. Other arguments go to bazel test.
#
_bazel_regression()
{
    local regex="."
    if [[ ${1:-} == -R ]]; then
        if [ $# -lt 2 ]; then
            echo "regression: -R needs a regex" >&2
            exit 2
        fi
        regex=$2
        shift 2
    fi

    local bazel=bazelisk
    if ! command -v "$bazel" >/dev/null 2>&1; then
        bazel=bazel
    fi
    if ! command -v "$bazel" >/dev/null 2>&1; then
        echo "regression: neither bazelisk nor bazel is on PATH" >&2
        exit 1
    fi

    # The bazel query lexer has no escapes, so pick a quote that the regex
    # does not contain.
    local q
    if [[ "$regex" != *'"'* ]]; then
        q='"'
    elif [[ "$regex" != *"'"* ]]; then
        q="'"
    else
        echo "regression: the -R regex cannot contain both \" and '" >&2
        exit 2
    fi

    # The query regex is matched against the full label, for example
    # //openroad/rsz/test:repair_tie12_hier-tcl_test.
    # Tests tagged "manual" are left out, as in "bazel test //openroad/<tool>/...".
    # Bazel query matches a list attribute as the string "[a, b]", so the
    # tags regex matches only a whole "manual" element.
    local pkg="//openroad/${tool}/..."
    local query="filter(${q}${regex}${q}, kind(\".*_test rule\", ${pkg}))"
    query+=' except attr("tags", "[\[ ]manual[,\]]", '"${pkg})"
    local targets
    targets=$("$bazel" query --output=label "$query")
    if [ -z "$targets" ]; then
        echo "regression: no test in //openroad/${tool}/... matches '${regex}'" >&2
        exit 1
    fi

    local target_list=()
    while IFS= read -r target; do
        target_list+=("$target")
    done <<< "$targets"
    echo "regression: running ${#target_list[@]} test(s) that match '${regex}'"
    # Targets named on the command line fail when incompatible with the
    # platform (for example GPU-only tests), so skip them instead.
    "$bazel" test --skip_incompatible_explicit_targets "$@" "${target_list[@]}"
}

_get_tool_name

_bazel_regression "$@"
