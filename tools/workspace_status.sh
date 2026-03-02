#!/usr/bin/env bash
# Outputs key-value pairs consumed by Bazel workspace stamping.
# Keys prefixed with STABLE_ cause downstream rebuilds when their value changes.
# The OpenRoadVersion genrule uses stamp = 1, so these values are always
# embedded.  No global --stamp flag is needed.
GIT_VERSION=$(git describe --tags --match '[0-9][0-9]Q[0-9]' --always 2>/dev/null \
  || echo "unknown")
echo "STABLE_GIT_VERSION ${GIT_VERSION}"
