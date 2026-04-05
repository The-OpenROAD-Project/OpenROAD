#!/usr/bin/env bash
set -e

# Install binary and runfiles from bazel build

TARFILE=$(cd $BUILD_WORKSPACE_DIRECTORY; bazelisk info bazel-bin)/packaging/openroad.tar

DEST_DIR=${1:-${BUILD_WORKSPACE_DIRECTORY}/../install/OpenROAD/bin}

mkdir -p "$DEST_DIR"
cp -f "$TARFILE" "$DEST_DIR"
cd "$DEST_DIR"
tar -xf openroad.tar
rm -f openroad.tar

# Remove useless files from pkg_tar from bazel
if [ -e openroad.repo_mapping ]; then
    chmod u+w openroad.repo_mapping
    rm -rf openroad.repo_mapping
fi
rm -rf openroad.runfiles/_main

echo "OpenROAD binary installed to $(realpath "$DEST_DIR")"
