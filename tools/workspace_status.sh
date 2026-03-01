#!/usr/bin/env bash
# Outputs key-value pairs consumed by Bazel workspace stamping.
# Keys prefixed with STABLE_ cause downstream rebuilds when their value changes.
# With --nostamp (default), Bazel substitutes empty strings, keeping the cache intact.
GIT_VERSION=$(git describe --tags --match '[0-9][0-9]Q[0-9]' 2>/dev/null \
  || git rev-parse --short HEAD 2>/dev/null \
  || echo "unknown")
echo "STABLE_GIT_VERSION ${GIT_VERSION}"
