#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# Start the OpenROAD web viewer in the background on a design.
#
#   bash .agents/skills/run-web-viewer/launch.sh <out_dir> [load.tcl]
#
# Without load.tcl it loads the repo's routed Nangate45 gcd.  load.tcl must
# read liberty, the database, and the SDC; this script appends web_server.
# openroad runs in the directory launch.sh was started from, so relative
# paths in load.tcl resolve from there.  load.tcl must not be inside
# <out_dir>, which must be new, or an empty directory you own; it is made
# private (0700).  Writes <out_dir>/server.pid, <out_dir>/token, and
# <out_dir>/or.log, and prints the viewer URL once the server listens.
#
# Env: WEB_PORT (8765), WEB_TIMEOUT (seconds to wait for the server, 60),
# OPENROAD_EXE (the binary, <repo>/bazel-bin/openroad).
set -euo pipefail
# web.tcl and token hold the token: keep every file private to this user, and
# never write through a file or symlink that is already there
umask 077
set -o noclobber

out=${1:?usage: launch.sh <out_dir> [load.tcl]}
load=${2:-}
repo=$(cd "$(dirname "$0")/../../.." && pwd)
port=${WEB_PORT:-8765}
timeout=${WEB_TIMEOUT:-60}
openroad=${OPENROAD_EXE:-$repo/bazel-bin/openroad}

# Check every input before creating <out_dir>: a failed launch leaves it
# non-empty, and a non-empty one is refused next time.
if [ ! -f "$openroad" ] || [ ! -x "$openroad" ]; then
  echo "no openroad binary at $openroad: bazelisk build //:openroad," \
       "or set OPENROAD_EXE" >&2
  exit 1
fi
if [ -n "$load" ] && [ ! -r "$load" ]; then
  echo "cannot read $load" >&2
  exit 1
fi
if (exec 3<>"/dev/tcp/127.0.0.1/$port") 2>/dev/null; then
  echo "port $port is in use: stop.sh the old viewer or set WEB_PORT" >&2
  exit 1
fi

# A reused or shared directory could hold a symlink or a readable file where
# the token goes: only a new or empty directory of this user's is accepted.
if [ -e "$out" ] || [ -L "$out" ]; then
  if [ -L "$out" ] || [ ! -d "$out" ] || [ ! -O "$out" ] \
     || [ -n "$(ls -A -- "$out")" ]; then
    echo "out_dir must be new, or an empty directory you own: $out" >&2
    exit 1
  fi
  chmod 700 "$out"
else
  mkdir -p "$(dirname "$out")"
  mkdir -m 700 "$out"
fi
out=$(cd "$out" && pwd)

# The token guards the viewer's Tcl console, which runs any command as this
# user: a fresh random one per launch.
token=$(od -An -tx1 -N16 /dev/urandom | tr -d ' \n')
echo "$token" > "$out/token"

# Paths reach Tcl through the environment, not spliced into the script, so
# spaces and braces in them need no quoting.
export WEB_REPO=$repo WEB_PID_FILE=$out/server.pid
if [ -z "$load" ]; then
  load=$out/load_gcd.tcl
  cat > "$load" <<'EOF'
set repo $::env(WEB_REPO)
# the SDC calls set_all_input_output_delays, a test helper
source $repo/test/flow_helpers.tcl
read_lef $repo/test/Nangate45/Nangate45.lef
read_liberty $repo/test/Nangate45/Nangate45_typ.lib
read_def $repo/src/odb/test/data/gcd/gcd_nangate45_route.def
read_sdc $repo/test/gcd_nangate45.sdc
EOF
fi

script=$out/web.tcl
# openroad records its own pid, the one stop.sh checks and kills
echo 'set f [open $::env(WEB_PID_FILE) w]; puts $f [pid]; close $f' \
  > "$script"
cat "$load" >> "$script"
echo "web_server -port $port -no_browser -token $token" >> "$script"

# The web assets are compiled into the binary: rebuild after editing src/web.
# exec: the background subshell becomes openroad, so no shell lingers holding
# this script's stdout open (a pipe from launch.sh would never close).
(exec nohup "$openroad" "$script" > "$out/or.log" 2>&1 < /dev/null) &
echo $! > "$out/launcher.pid"

for _ in $(seq "$timeout"); do
  if grep -qs "WEB-0001" "$out/or.log"; then
    echo "http://localhost:$port/?token=$token"
    exit 0
  fi
  if ! kill -0 "$(cat "$out/launcher.pid")" 2>/dev/null; then
    echo "openroad exited; see $out/or.log" >&2
    tail -20 "$out/or.log" >&2
    exit 1
  fi
  sleep 1
done
echo "server did not start in $timeout s (set WEB_TIMEOUT); see $out/or.log" >&2
exit 1
