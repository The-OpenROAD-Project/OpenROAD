#!/bin/bash
set -e
BUILDIFIER_BIN=$(realpath "$1")
if [ -n "${BUILD_WORKSPACE_DIRECTORY:-}" ]; then
  cd "$BUILD_WORKSPACE_DIRECTORY"
fi
"$BUILDIFIER_BIN" -r -mode=check -lint=warn .
