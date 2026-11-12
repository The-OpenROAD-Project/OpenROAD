#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, Precision Innovations Inc.

# Checks that clang-tidy ran cleanly under the rules_lint aspect and
# writes its diagnostics, one `path:line:col: warning|error: ...` line
# each, with workspace-relative paths.
#
# Usage: collect_clang_tidy.sh <bazel-bin> <output-file>
#
# rules_lint records clang-tidy's exit code instead of failing the build,
# so a broken linter would otherwise look like "0 findings". With
# WarningsAsErrors off in //:.clang-tidy, findings exit 0, and compiler
# errors fail the bazel build in rules_lint's wrapper. Any other non-zero
# recorded exit (bad binary, bad flags, bad config, crash) is a failure.
#
# Only reads outputs present in <bazel-bin>; callers delete stale ones
# before the build so the results reflect this build only.

set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <bazel-bin> <output-file>" >&2
  exit 2
fi
bazel_bin=$1
output=$2

mapfile -d '' outs < <(
  find "${bazel_bin}" -name '*.AspectRulesLintClangTidy.out' -print0
)
if [[ ${#outs[@]} -eq 0 ]]; then
  echo "::error::No clang-tidy outputs found under ${bazel_bin}"
  exit 1
fi

failures=0
for out in "${outs[@]}"; do
  if [[ -f "${out}.exit_code" ]]; then
    code=$(<"${out}.exit_code")
  else
    code="missing"
  fi
  if [[ "${code}" != "0" ]]; then
    failures=$((failures + 1))
    if [[ ${failures} -le 5 ]]; then
      echo "::error::clang-tidy failed (exit ${code}) for ${out#"${bazel_bin}"/}"
      head -5 "${out}"
    fi
  fi
done
echo "clang-tidy outputs: ${#outs[@]}, tool failures: ${failures}"
if [[ ${failures} -gt 0 ]]; then
  echo "::error::clang-tidy did not run correctly"
  exit 1
fi

# Paths in .out files are sandbox-absolute; strip to workspace-relative
# so reviewdog can match against the PR diff. Keep only
# `path:line:col: warning|error:` lines, which drops source-context
# carets, notes, and clang-tidy's header noise. Drop findings against
# external and generated files (not in any PR diff). `sort -u` dedupes a
# finding emitted under several cc_library consumers of a shared source.
printf '%s\0' "${outs[@]}" \
  | xargs -0 cat \
  | sed -E 's|^.*/execroot/_main/||' \
  | { grep -E '^[^:]+:[0-9]+:[0-9]+: (warning|error):' || true; } \
  | { grep -vE '^(bazel-out|external)/' || true; } \
  | sort -u \
  > "${output}"
echo "Findings: $(wc -l < "${output}")"
