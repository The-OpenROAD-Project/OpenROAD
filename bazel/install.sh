#!/usr/bin/env bash
set -e

# Install binary and runfiles from bazel build.
#
# ORFS context: When OpenROAD is checked out as a submodule of
# OpenROAD-flow-scripts at tools/OpenROAD/, the ORFS Make flow expects
# the binary at tools/install/OpenROAD/bin/openroad. This path
# originates from build_openroad.sh's CMAKE_INSTALL_PREFIX and is
# hardcoded in flow/scripts/variables.mk:
#
#   export OPENROAD_EXE ?= $(abspath $(FLOW_HOME)/../tools/install/OpenROAD/bin/openroad)
#
# The Bazel install target was created as a drop-in replacement for
# the CMake install step, so the suggested ORFS path below matches
# that convention.

# Support direct execution outside of bazel run
BUILD_WORKSPACE_DIRECTORY="${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

TARFILE="${TARFILE:-$(cd "$BUILD_WORKSPACE_DIRECTORY" && bazelisk info bazel-bin)/openroad.tar}"

if [ "$1" == "-f" ]; then
    DEST_DIR="${BUILD_WORKSPACE_DIRECTORY}/../install/OpenROAD/bin"
elif [ "$#" -gt 0 ]; then
    DEST_DIR="$1"
else
    echo "Error: Please specify an installation path."
    echo ""
    echo "Examples:"
    echo "  bazel run :install -- ~/.local/bin    # Add to your PATH"
    echo "  bazel run :install -- ./build/install  # Project-local install"
    echo ""
    echo "If you are using ORFS (OpenROAD-flow-scripts), you can use the -f flag"
    echo "to install to the path that flow/scripts/variables.mk expects:"
    echo ""
    echo "  bazel run :install -- -f"
    exit 1
fi

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
