#!/usr/bin/env bash
# Wrapper for whittle.py that sets OPENROAD_EXE from Bazel runfiles.
# Usage: bazelisk run //:whittle -- <whittle.py args>
set -euo pipefail

RUNFILES="${BASH_SOURCE[0]}.runfiles/_main"

if [[ -z "${OPENROAD_EXE:-}" ]]; then
    OPENROAD_EXE="$RUNFILES/openroad"
fi
export OPENROAD_EXE
export PATH="$(dirname "$(realpath "$OPENROAD_EXE")"):$PATH"

exec python3 "$RUNFILES/etc/whittle.py" "$@"
