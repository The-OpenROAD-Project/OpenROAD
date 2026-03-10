#!/usr/bin/env bash
#
# Setup ORFS dependencies for developers using Bazel for OpenROAD.
#
# Invoked via: bazelisk run //:setup-orfs
#   (from the tools/OpenROAD directory)
#
# This script:
#   1. Checks if system packages are installed
#   2. If not, prints the sudo command to run and exits
#   3. If yes, builds Yosys/eqy/sby and installs pip packages with
#      user permissions into $ORFS_DIR/dependencies
#
# Shares code with ORFS etc/DependencyInstaller.sh via the -bazel flag.

set -euo pipefail

OR_DIR="${BUILD_WORKSPACE_DIRECTORY:?Must be run via bazelisk run}"

# Navigate from tools/OpenROAD to the ORFS root
ORFS_DIR="$(cd "${OR_DIR}/../.." && pwd)"

if [[ ! -f "${ORFS_DIR}/etc/DependencyInstaller.sh" ]]; then
    echo "ERROR: Cannot find ORFS root at ${ORFS_DIR}"
    echo "This script expects OpenROAD to be at tools/OpenROAD within ORFS."
    exit 1
fi

cd "${ORFS_DIR}"

# Check for key system packages needed to build Yosys and run the flow.
# These are installed by: sudo ./etc/DependencyInstaller.sh -base -bazel
missing=()
for cmd in gcc g++ bison flex pkg-config tclsh python3; do
    if ! command -v "$cmd" &>/dev/null; then
        missing+=("$cmd")
    fi
done

# Also check for key dev libraries via pkg-config
for lib in tcl libffi zlib; do
    if ! pkg-config --exists "$lib" 2>/dev/null; then
        missing+=("$lib (dev)")
    fi
done

if [[ ${#missing[@]} -gt 0 ]]; then
    echo "Missing system dependencies: ${missing[*]}"
    echo ""
    echo "Run the following command first to install system packages:"
    echo ""
    echo "  sudo ${ORFS_DIR}/etc/DependencyInstaller.sh -base -bazel"
    echo ""
    exit 1
fi

PREFIX="${ORFS_DIR}/dependencies"

echo "Installing ORFS flow dependencies into ${PREFIX}"
echo ""

# Build Yosys/eqy/sby using the shared DependencyInstaller.sh with -bazel.
# The -common flag triggers the Yosys/eqy/sby build (not the full OpenROAD
# dependency chain).
"${ORFS_DIR}/etc/DependencyInstaller.sh" -common -bazel "-prefix=${PREFIX}"

echo ""
echo "ORFS flow dependencies installed to ${PREFIX}"
echo ""
echo "To use, source the environment or add to PATH:"
echo ""
echo "  export PATH=${PREFIX}/bin:\$PATH"
echo ""
