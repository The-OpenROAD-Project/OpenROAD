#!/bin/bash
set -e

# Install binary and runfiles from bazel build

# Check for required system dependencies before expensive build.
# Each tool checks its own deps (separation of concerns).
#
# Currently only Ubuntu/Debian is checked. Dependency checking for
# other platforms (macOS, RHEL, Fedora, etc.) is not implemented
# because we cannot test them. Contributions welcome.
check_ubuntu_deps() {
    local missing=()
    # GUI deps (xcb libraries needed for Qt)
    for pkg in libxcb-icccm4-dev libxcb-image0-dev libxcb-keysyms1-dev \
               libxcb-render-util0-dev libxcb-xinerama0-dev libxcb-xkb-dev; do
        if ! dpkg -s "$pkg" &>/dev/null 2>&1; then
            missing+=("$pkg")
        fi
    done
    if [[ ${#missing[@]} -gt 0 ]]; then
        echo "ERROR: Missing dependencies for OpenROAD GUI build."
        echo ""
        echo "On Ubuntu this would be:"
        echo "  sudo apt install ${missing[*]}"
        exit 1
    fi
}

if command -v dpkg &>/dev/null; then
    check_ubuntu_deps
fi

TARFILE=$(cd $BUILD_WORKSPACE_DIRECTORY; bazelisk info bazel-bin)/openroad.tar

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
