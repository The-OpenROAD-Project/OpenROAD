#!/bin/bash
# Verify that pairs of ORFS output files are identical.
# Args come in pairs: file_a file_b file_a file_b ...
set -euo pipefail
status=0
while [ $# -ge 2 ]; do
    a="$1"; b="$2"; shift 2
    if ! diff -q "$a" "$b" > /dev/null 2>&1; then
        echo "DIFFER: $(basename "$a") vs $(basename "$b")"
        diff "$a" "$b" | head -20
        status=1
    else
        echo "OK: $(basename "$a") and $(basename "$b") are identical"
    fi
done
exit $status
