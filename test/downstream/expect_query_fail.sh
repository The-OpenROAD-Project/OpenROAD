#!/bin/bash
# Verify that a bazelisk query FAILS (dep not available to downstream).
# Usage: expect_query_fail.sh <query>
set -e -u -o pipefail
cd "$(dirname "$(readlink -f "$0")")"
export HOME="${HOME:-$(getent passwd "$(id -u)" | cut -d: -f6)}"
if bazelisk query "$1" 2>/dev/null; then
    echo "FAIL: $1 should not be available"
    exit 1
fi
