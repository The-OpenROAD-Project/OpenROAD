#!/bin/bash
# Verify that a target in the downstream workspace succeeds --nobuild analysis.
# Usage: expect_build.sh <target>
set -e -u -o pipefail
cd "$(dirname "$(readlink -f "$0")")"
export HOME="${HOME:-$(getent passwd "$(id -u)" | cut -d: -f6)}"
exec bazelisk build --nobuild "$1"
