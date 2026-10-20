#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

set -e

#
# Uses the current directory to figure out what the tool name is. Allows us to
# call the script from src/tool/test and src/tool directories. Wish we could
# just use the path of the regression script, but we can't since $0 points to
# the location in test/shared and not src/tool/test.
#
_get_tool_name()
{
    echo $(readlink $0)
    script_path=$(realpath $PWD)
    tool=$(basename "${script_path%%/src/*}/src/$(echo "$script_path" | awk -F'/src/' '{print $2}' | cut -d'/' -f1)")
}

#
# Usage: ./regression [-R <regex>] [bazel test args...]
# Selects the test targets below //src/<tool>/ whose label matches <regex>
# and runs them in one bazel test call. Other arguments go to bazel test.
#
_bazel_regression()
{
    local regex="."
    local bazel_args=()
    while [ $# -gt 0 ]; do
        case "$1" in
            -R)
                if [ $# -lt 2 ]; then
                    echo "regression: -R needs a regex" >&2
                    exit 2
                fi
                regex=$2
                shift 2
                ;;
            *)
                bazel_args+=("$1")
                shift
                ;;
        esac
    done

    local bazel=bazelisk
    if ! command -v "$bazel" >/dev/null 2>&1; then
        bazel=bazel
    fi
    if ! command -v "$bazel" >/dev/null 2>&1; then
        echo "regression: neither bazelisk nor bazel is on PATH" >&2
        exit 1
    fi

    # The query regex is matched against the full label, for example
    # //src/rsz/test:repair_tie12_hier-tcl_test.
    local query="filter(\"${regex}\", kind(\".*_test rule\", //src/${tool}/...))"
    local targets
    targets=$("$bazel" query --output=label "$query")
    if [ -z "$targets" ]; then
        echo "regression: no test in //src/${tool}/... matches '${regex}'" >&2
        exit 1
    fi

    local target_list=()
    while IFS= read -r target; do
        target_list+=("$target")
    done <<< "$targets"
    echo "regression: running ${#target_list[@]} test(s) that match '${regex}'"
    "$bazel" test "${bazel_args[@]}" -- "${target_list[@]}"
}

_get_tool_name

_bazel_regression "$@"
