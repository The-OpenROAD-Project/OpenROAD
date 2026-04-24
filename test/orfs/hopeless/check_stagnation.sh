#!/usr/bin/env bash
# Asserts that the repair_timing WNS-stagnation gate fired somewhere in the
# flow logs. The hopeless test design is specifically engineered so that
# repair_timing will iterate past the 1000-pass warmup on an infeasible
# clock target; if the gate stops firing for any reason (design got too
# small, warmup got longer, etc.) this test fails loudly instead of silently
# losing coverage.
set -euo pipefail

needle="obviously futile"
hits=0
for log in "$@"; do
  if grep -q "$needle" "$log"; then
    echo "FOUND in $log:"
    grep "$needle" "$log"
    hits=$((hits + 1))
  fi
done

if [[ "$hits" -eq 0 ]]; then
  echo "ERROR: repair_timing WNS-stagnation gate did not fire in any log." >&2
  echo "Searched $# log file(s) for '$needle'." >&2
  echo "This means either:" >&2
  echo "  (a) the hopeless.v design is no longer pathological enough," >&2
  echo "  (b) the gate's warmup/threshold changed, or" >&2
  echo "  (c) the log message was renamed." >&2
  exit 1
fi

echo "OK: WNS-stagnation gate fired in $hits log file(s)."
