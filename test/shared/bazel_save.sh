#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

# Bazel-mode helper used by save_ok / save_defok / save_guideok.
# Each test's artifact lives under bazel-testlogs/<package>/<name>-<lang>_test/
# either as test.outputs/results/<name>-<lang>.<ext> (unzipped, current
# bazel default) or test.outputs/outputs.zip (zipped, older bazel).
#
# Must be run from the test source directory; the enclosing bazel package is
# derived from $PWD.
#
# Usage: bazel_save.sh [--no-run] <dest_ext> <src_ext> <test_name>...
#        bazel_save.sh [--no-run] <dest_ext>:<src_ext>... <test_name>...
#   dest_ext   Extension to write next to the test (e.g. ok, defok, guideok).
#   src_ext    Artifact extension under results/ (log, def, guide).
#   test_name  One or more test stems. Targets are derived as
#              //<package>:<name>-tcl_test then //<package>:<name>-py_test.
#   --no-run   Harvest whatever is already in bazel-testlogs; skip `bazel test`.
#
# The colon form saves several goldens from a single test run, e.g.
#   bazel_save.sh ok:log defok:def large01 macro01
#
# All tests are passed to ONE `bazel test` invocation, because bazel only
# parallelizes within an invocation -- a loop of single-target runs is serial.
#
# NOTE: a .ok log embeds the result of the test's own DEF comparison
# ("No differences found." vs "Differences found at line N."), so a .defok
# must already be correct at the time of the run that produces the .ok.
# When placement output shifts, updating both takes two passes:
#   bazel_save.sh defok:def <tests>   # run 1, fix the DEF golden
#   bazel_save.sh ok:log <tests>      # run 2, log now reports a clean compare

set -e

run_tests=1
if [ "${1:-}" = "--no-run" ]; then
    run_tests=0
    shift
fi

usage() {
    echo "usage: $0 [--no-run] <dest_ext> <src_ext> <test_name>..." >&2
    echo "       $0 [--no-run] <dest_ext>:<src_ext>... <test_name>..." >&2
    exit 2
}

# Leading args carrying a colon are dest:src pairs; otherwise fall back to the
# legacy two-positional form so existing callers keep working unchanged.
pairs=()
case "${1:-}" in
    *:*)
        while [ $# -gt 0 ]; do
            case "$1" in
                *:*) pairs+=("$1"); shift ;;
                *) break ;;
            esac
        done
        ;;
    *)
        [ $# -ge 3 ] || usage
        pairs=("$1:$2")
        shift 2
        ;;
esac

[ ${#pairs[@]} -gt 0 ] && [ $# -ge 1 ] || usage

if ! command -v bazel >/dev/null 2>&1; then
    echo "bazel not on PATH; cannot extract goldens from bazel-testlogs" >&2
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

# Resolve each stem to its language variant up front, with one query for the
# whole package, so the run below can be a single batched invocation.
existing=$(bazel query "kind('.*_test', //${pkg}:all)" 2>/dev/null || true)

declare -A test_lang=()
targets=()
for test_name in "$@"; do
    for lang_ext in tcl py; do
        target="//${pkg}:${test_name}-${lang_ext}_test"
        if printf '%s\n' "$existing" | grep -qxF "$target"; then
            test_lang["$test_name"]=$lang_ext
            targets+=("$target")
            break
        fi
    done
    if [ -z "${test_lang[$test_name]:-}" ]; then
        echo "\"${test_name}\" has no -tcl_test or -py_test target in //${pkg}" >&2
    fi
done

if [ ${#targets[@]} -eq 0 ]; then
    echo "no matching test targets in //${pkg}" >&2
    exit 1
fi

if [ "$run_tests" -eq 1 ]; then
    # One invocation so bazel runs them in parallel. Failure is expected --
    # a mismatched golden is why we are here -- but build errors and
    # target-not-found stay visible on stderr.
    #
    # cache_test_results=yes because bazel's `auto` default re-runs any test
    # that failed last time, which is precisely every test being saved here;
    # without it the harvest pays for a second full run.
    bazel test "${targets[@]}" \
        --cache_test_results=yes \
        --test_summary=terse >/dev/null || true
fi

# Preserve the golden's current permissions: bazel-testlogs artifacts are
# read-only and `cp` carries that mode over, which would both break the next
# save and churn the executable bit git tracks on .ok files.
restore_mode() {
    local dst=$1 mode=$2
    if [ -n "$mode" ]; then
        chmod "$mode" "$dst"
    else
        chmod u+w "$dst"
    fi
}

saved_count=0
for test_name in "$@"; do
    lang_ext=${test_lang[$test_name]:-}
    [ -n "$lang_ext" ] || continue
    out_dir="${testlogs}/${pkg}/${test_name}-${lang_ext}_test/test.outputs"

    for pair in "${pairs[@]}"; do
        dest_ext=${pair%%:*}
        src_ext=${pair##*:}
        artifact="results/${test_name}-${lang_ext}.${src_ext}"
        dst="${test_name}.${dest_ext}"

        mode=""
        if [ -f "$dst" ]; then
            mode=$(stat -c%a "$dst" 2>/dev/null \
                || stat -f%Lp "$dst" 2>/dev/null \
                || echo "")
        fi

        if [ -f "${out_dir}/${artifact}" ]; then
            rm -f "$dst"
            cp "${out_dir}/${artifact}" "$dst"
            restore_mode "$dst" "$mode"
            echo "${test_name}.${dest_ext}"
            saved_count=$((saved_count + 1))
            continue
        fi

        zip="${out_dir}/outputs.zip"
        if [ -f "$zip" ]; then
            tmp="${dst}.tmp"
            if unzip -p "$zip" "$artifact" > "$tmp" 2>/dev/null \
                    && [ -s "$tmp" ]; then
                rm -f "$dst"
                mv "$tmp" "$dst"
                restore_mode "$dst" "$mode"
                echo "${test_name}.${dest_ext}"
                saved_count=$((saved_count + 1))
                continue
            fi
            rm -f "$tmp"
        fi

        echo "\"${test_name}\" ${src_ext} file not found in bazel-testlogs" >&2
    done
done

[ "$saved_count" -gt 0 ] || exit 1
