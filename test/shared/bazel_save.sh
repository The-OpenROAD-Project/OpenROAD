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
#              It cannot tell whether those outputs are current. When a test
#              has both variants, the tcl output wins over the py one.
#
# Exits nonzero if any requested golden could not be saved.
#
# All tests are passed to ONE `bazel test` invocation, because bazel only
# parallelizes within an invocation -- a loop of single-target runs is serial.
#
# The colon form saves several goldens from one run, e.g.
#   bazel_save.sh ok:log defok:def large01 macro01
# A .ok log embeds the test's own DEF comparison ("No differences found."
# vs "Differences found at line N."), so when the .defok changes, the .ok
# saved alongside it records a mismatch against the old one. Run the same
# command a second time: the new .defok invalidates the cached result and
# the re-run log reports a clean compare.

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
# whole package, so the run below can be a single batched invocation. A run
# uses the first variant found; --no-run keeps both, since either may hold the
# outputs.
existing=$(bazel query "kind('.*_test', //${pkg}:all)" 2>/dev/null || true)

missing=0
targets=()
for test_name in "$@"; do
    found=0
    for lang_ext in tcl py; do
        target="//${pkg}:${test_name}-${lang_ext}_test"
        if printf '%s\n' "$existing" | grep -qxF "$target"; then
            targets+=("$target")
            found=1
            [ "$run_tests" -eq 0 ] || break
        fi
    done
    if [ "$found" -eq 0 ]; then
        echo "\"${test_name}\" has no -tcl_test or -py_test target in //${pkg}" >&2
        missing=1
    fi
done

if [ ${#targets[@]} -eq 0 ]; then
    echo "no matching test targets in //${pkg}" >&2
    exit 1
fi

if [ "$run_tests" -eq 1 ]; then
    # One invocation so bazel runs them in parallel. Exit 3 (build ok, tests
    # failed) is expected -- a mismatched golden is why we are here. Any other
    # failure means some targets may not have run, leaving stale testlogs.
    #
    # cache_test_results=yes because bazel's `auto` default re-runs any test
    # that failed last time, which is precisely every test being saved here;
    # without it the harvest pays for a second full run.
    rc=0
    bazel test "${targets[@]}" \
        --cache_test_results=yes \
        --test_summary=terse >/dev/null || rc=$?
    if [ "$rc" -ne 0 ] && [ "$rc" -ne 3 ]; then
        echo "bazel test failed (exit ${rc}); no goldens saved" >&2
        exit "$rc"
    fi
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

for test_name in "$@"; do
    for pair in "${pairs[@]}"; do
        dest_ext=${pair%%:*}
        src_ext=${pair##*:}
        dst="${test_name}.${dest_ext}"

        mode=""
        if [ -f "$dst" ]; then
            mode=$(stat -c%a "$dst" 2>/dev/null \
                || stat -f%Lp "$dst" 2>/dev/null \
                || echo "")
        fi

        saved=0
        for lang_ext in tcl py; do
            target="//${pkg}:${test_name}-${lang_ext}_test"
            printf '%s\n' "${targets[@]}" | grep -qxF "$target" || continue
            out_dir="${testlogs}/${pkg}/${test_name}-${lang_ext}_test/test.outputs"
            artifact="results/${test_name}-${lang_ext}.${src_ext}"

            if [ -f "${out_dir}/${artifact}" ]; then
                rm -f "$dst"
                cp "${out_dir}/${artifact}" "$dst"
                restore_mode "$dst" "$mode"
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
                    restore_mode "$dst" "$mode"
                    saved=1
                    break
                fi
                rm -f "$tmp"
            fi
        done

        if [ "$saved" -eq 1 ]; then
            echo "$dst"
        else
            echo "\"${test_name}\" ${src_ext} file not found in bazel-testlogs" >&2
            missing=1
        fi
    done
done

exit "$missing"
