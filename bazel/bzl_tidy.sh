#!/bin/bash
set -e
BUILDIFIER_BIN=$(realpath "$1")

if [ -z "$BUILD_WORKSPACE_DIRECTORY" ]; then
  echo "Error: This script must be run via 'bazelisk run'"
  exit 1
fi

cd "$BUILD_WORKSPACE_DIRECTORY"
"$BUILDIFIER_BIN" -r -mode=fix -lint=fix .
echo "Buildifier tidy complete!"
