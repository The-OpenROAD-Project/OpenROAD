#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

# Bazel-mode helper used by save_ok / save_defok / save_guideok.
# Each test's artifact lives under bazel-testlogs/<package>/<name>-<lang>_test/
# either as test.outputs/results/<name>-<lang>.<ext> (unzipped, current
# bazel default) or test.outputs/outputs.zip (zipped, older bazel).
#
# Usage: bazel_save.sh <dest_ext> <src_ext> <test_name>...
#   dest_ext   Extension to write next to the test (e.g. ok, defok, guideok).
#   src_ext    Artifact extension under results/ (log, def, guide).
#   test_name  One or more test stems. Targets are derived as
#              //<package>:<name>-tcl_test then //<package>:<name>-py_test.

set -e

if [ $# -lt 3 ]; then
    echo "usage: $0 <dest_ext> <src_ext> <test_name>..." >&2
    exit 2
fi

dest_ext=$1
src_ext=$2
shift 2

if ! command -v bazel >/dev/null 2>&1; then
    echo "bazel not on PATH; cannot extract .${dest_ext} from bazel-testlogs" >&2
    exit 1
fi

testlogs=$(bazel info bazel-testlogs 2>/dev/null || true)
if [ -z "$testlogs" ]; then
    echo "not inside a bazel workspace" >&2
    exit 1
fi

# Ask bazel for the enclosing package label rather than computing it
# from a path; that's both portable (no GNU `realpath --relative-to`)
# and authoritative if BUILD files ever move.
pkg=$(bazel query --output=package ':all' 2>/dev/null | head -1)
if [ -z "$pkg" ]; then
    echo "no bazel package found at $PWD" >&2
    exit 1
fi

for test_name in "$@"; do
    saved=0
    for lang_ext in tcl py; do
        target="//${pkg}:${test_name}-${lang_ext}_test"
        out_dir="${testlogs}/${pkg}/${test_name}-${lang_ext}_test/test.outputs"
        artifact="results/${test_name}-${lang_ext}.${src_ext}"
        # Run the test so the cache reflects the current code; failure
        # is expected (mismatched .ok is why we're here). Build errors
        # and target-not-found stay visible on stderr.
        bazel test "$target" --test_summary=terse >/dev/null || true
        # Bazel-testlogs files are read-only and `cp` preserves mode,
        # so force-replace and chmod so the next iteration can overwrite.
        dst="${test_name}.${dest_ext}"
        if [ -f "${out_dir}/${artifact}" ]; then
            rm -f "$dst"
            cp "${out_dir}/${artifact}" "$dst"
            chmod u+w "$dst"
            echo "${test_name}"
            saved=1
            break
        fi
        zip="${out_dir}/outputs.zip"
        if [ -f "$zip" ]; then
            tmp="${dst}.tmp"
            if unzip -p "$zip" "$artifact" > "$tmp" 2>/dev/null \
                    && [ -s "$tmp" ]; then
                rm -f "$dst"
                mv "$tmp" "$dst"
                chmod u+w "$dst"
                echo "${test_name}"
                saved=1
                break
            fi
            rm -f "$tmp"
        fi
    done
    if [ "$saved" -eq 0 ]; then
        echo "\"${test_name}\" ${src_ext} file not found in bazel-testlogs" >&2
    fi
done
