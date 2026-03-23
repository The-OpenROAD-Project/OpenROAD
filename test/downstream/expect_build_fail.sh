#!/bin/bash
# Verify that a target in the downstream workspace FAILS --nobuild analysis.
# Usage: expect_build_fail.sh <target>
set -e -u -o pipefail
cd "$(dirname "$(readlink -f "$0")")"
export HOME="${HOME:-$(getent passwd "$(id -u)" | cut -d: -f6)}"
if bazelisk build --nobuild "$1" 2>/dev/null; then
    echo "FAIL: $1 should not be visible"
    exit 1
fi
