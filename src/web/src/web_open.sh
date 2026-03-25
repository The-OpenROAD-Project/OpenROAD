#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Launch the OpenROAD web viewer for a design.
# Usage: bazelisk run //test/orfs/gcd:gcd_route_web

set -euo pipefail

# Locate the openroad binary and web assets in bazel runfiles
if [[ -n "${RUNFILES_DIR:-}" ]]; then
    RUNFILES="$RUNFILES_DIR"
elif [[ -d "$0.runfiles" ]]; then
    RUNFILES="$0.runfiles"
else
    echo "Error: cannot find runfiles directory" >&2
    exit 1
fi

OPENROAD="$RUNFILES/_main/openroad"
WEB_DIR="$(dirname "$RUNFILES/_main/src/web/src/index.html")"

if [[ ! -x "$OPENROAD" ]]; then
    echo "Error: openroad binary not found at $OPENROAD" >&2
    exit 1
fi
if [[ ! -f "$WEB_DIR/index.html" ]]; then
    echo "Error: web assets not found at $WEB_DIR" >&2
    exit 1
fi

# Find the ODB file: explicit arg, env var, or auto-discover from runfiles
ODB="${ODB_FILE:-${1:-}}"
if [[ -z "$ODB" ]]; then
    # Auto-discover: find the most advanced stage ODB in runfiles
    ODB=$(find "$RUNFILES/_main" -name "*.odb" -path "*/results/*" 2>/dev/null \
        | sort | tail -1)
fi
if [[ -z "$ODB" || ! -f "$ODB" ]]; then
    echo "Error: ODB file not found. Set ODB_FILE or pass as first argument." >&2
    exit 1
fi

PORT="${WEB_PORT:-8088}"
URL="http://localhost:$PORT"

echo "Starting web viewer on $URL"
echo "Design: $ODB"

# Kill any existing server on this port
if lsof -ti :"$PORT" &>/dev/null; then
    echo "Killing existing server on port $PORT"
    kill $(lsof -ti :"$PORT") 2>/dev/null || true
    sleep 0.5
fi

# Write a temp TCL script (openroad takes a file, not inline commands)
TCL_SCRIPT=$(mktemp /tmp/web_open_XXXXXX.tcl)
trap "rm -f $TCL_SCRIPT" EXIT
cat > "$TCL_SCRIPT" <<ENDTCL
read_db $ODB
web_server -port $PORT -dir $WEB_DIR
ENDTCL

# The web_server command opens the browser itself (web.cpp calls xdg-open).

# Export RUNFILES_DIR so openroad's C++ runfiles library finds tclreadline etc.
# Without this, it tries argv[0].runfiles which fails for symlinked binaries.
export RUNFILES_DIR="$RUNFILES"
exec "$OPENROAD" -no_init -threads max "$TCL_SCRIPT"
