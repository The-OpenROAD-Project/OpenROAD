#!/bin/bash
set -e
SOURCE_BINARY=$(cd $BUILD_WORKSPACE_DIRECTORY; bazelisk info bazel-bin)/openroad

DEST_DIR="$BUILD_WORKSPACE_DIRECTORY/../install/OpenROAD/bin"
DEST_FILE="$DEST_DIR/openroad"

mkdir -p "$DEST_DIR"
cp -f -L "$SOURCE_BINARY" "$DEST_FILE"
# runfiles
mkdir -p "$DEST_DIR/openroad.runfiles"
cp -f -L -r "$SOURCE_BINARY.runfiles/." "$DEST_DIR/openroad.runfiles/"
chmod +x "$DEST_FILE"

echo "OpenROAD binary installed to $(realpath "$DEST_FILE")"
