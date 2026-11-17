#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# Stop a viewer started by launch.sh and wait for its port to close.
#
#   bash .agents/skills/run-web-viewer/stop.sh <out_dir>
set -euo pipefail
out=${1:?usage: stop.sh <out_dir>}
if [ ! -s "$out/server.pid" ]; then
  echo "no server.pid in $out: no viewer is running from there" >&2
  exit 1
fi
out=$(cd "$out" && pwd)
pid=$(cat "$out/server.pid")

# A PID is reused once its process exits: signal it only while it is still
# the openroad that runs this directory's web.tcl.
is_viewer() {
  # ps, not /proc: macOS has no /proc
  ps -o args= -p "$pid" 2>/dev/null | grep -qF -- "$out/web.tcl"
}

if ! is_viewer; then
  rm -f "$out/server.pid" "$out/launcher.pid"
  echo "viewer $pid had already exited; removed its stale pid file"
  exit 0
fi
# it may exit between the check and the signal
kill "$pid" 2>/dev/null || true
# the server takes a few seconds to close its session and release the port
for _ in $(seq 30); do
  if ! is_viewer; then
    rm -f "$out/server.pid" "$out/launcher.pid"
    echo "stopped $pid"
    exit 0
  fi
  sleep 1
done
echo "$pid still running after 30 s" >&2
exit 1
