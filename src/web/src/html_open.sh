#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Generate a self-contained static HTML viewer and open it.
# No server needed — single file, works from file:// directly.
#
# Usage: bazelisk run //test/orfs/gcd:gcd_route_html

set -euo pipefail

if [[ -n "${RUNFILES_DIR:-}" ]]; then
    RUNFILES="$RUNFILES_DIR"
elif [[ -d "$0.runfiles" ]]; then
    RUNFILES="$0.runfiles"
else
    echo "Error: cannot find runfiles directory" >&2
    exit 1
fi

NODE=$(command -v node 2>/dev/null || true)
if [[ -z "$NODE" ]]; then
    echo "Error: node not found in PATH" >&2
    exit 1
fi

WEB_SRC="$RUNFILES/_main/src/web/src"
RENDER_PAGE="$WEB_SRC/render-static-page.js"
JSON=$(find "$RUNFILES/_main" -name "*.json" -path "*/test/orfs/*" 2>/dev/null | head -1)

if [[ ! -f "$RENDER_PAGE" ]]; then
    echo "Error: render-static-page.js not found" >&2
    exit 1
fi
if [[ -z "$JSON" ]]; then
    echo "Error: no JSON payload found" >&2
    exit 1
fi

if [[ -n "${BUILD_WORKSPACE_DIRECTORY:-}" ]]; then
    OUTPUT="$BUILD_WORKSPACE_DIRECTORY/gcd_report.html"
else
    OUTPUT="/tmp/gcd_report.html"
fi

"$NODE" "$RENDER_PAGE" "$JSON" -o "$OUTPUT"

echo "Opening $OUTPUT"
xdg-open "$OUTPUT" 2>/dev/null || open "$OUTPUT" 2>/dev/null || echo "Open $OUTPUT in your browser"
