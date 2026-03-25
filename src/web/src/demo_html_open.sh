#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Generate a demo static HTML report (histogram-only prototype) and open it.
# Usage: bazelisk run //test/orfs/gcd:gcd_route_demo_html

set -euo pipefail

if [[ -n "${RUNFILES_DIR:-}" ]]; then
    RUNFILES="$RUNFILES_DIR"
elif [[ -d "$0.runfiles" ]]; then
    RUNFILES="$0.runfiles"
else
    echo "Error: cannot find runfiles directory" >&2
    exit 1
fi

RENDER_STATIC="$RUNFILES/_main/src/web/render_static_/render_static"
if [[ ! -x "$RENDER_STATIC" ]]; then
    echo "Error: render_static not found at $RENDER_STATIC" >&2
    exit 1
fi

JSON=$(find "$RUNFILES/_main" -name "*.json" -path "*/test/orfs/*" 2>/dev/null | head -1)
if [[ -z "$JSON" ]]; then
    echo "Error: no JSON payload found in runfiles" >&2
    exit 1
fi

if [[ -n "${BUILD_WORKSPACE_DIRECTORY:-}" ]]; then
    OUTPUT="$BUILD_WORKSPACE_DIRECTORY/gcd_demo_report.html"
else
    OUTPUT="/tmp/gcd_demo_report.html"
fi

"$RENDER_STATIC" --label route "$JSON" -o "$OUTPUT"

echo "Opening $OUTPUT"
xdg-open "$OUTPUT" 2>/dev/null || open "$OUTPUT" 2>/dev/null || echo "Open $OUTPUT in your browser"
