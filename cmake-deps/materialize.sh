#!/usr/bin/env bash

set -eu

# --- begin runfiles.bash initialization v3 ---
set -uo pipefail
set +e
f=bazel_tools/tools/bash/runfiles/runfiles.bash
# shellcheck disable=SC1090
source "${RUNFILES_DIR:-/dev/null}/$f" 2>/dev/null || \
  source "$(grep -sm1 "^$f " "${RUNFILES_MANIFEST_FILE:-/dev/null}" | cut -f2- -d' ')" 2>/dev/null || \
  source "$0.runfiles/$f" 2>/dev/null || \
  source "$(grep -sm1 "^$f " "$0.runfiles_manifest" | cut -f2- -d' ')" 2>/dev/null || \
  source "$(grep -sm1 "^$f " "$0.exe.runfiles_manifest" | cut -f2- -d' ')" 2>/dev/null || \
  { echo>&2 "ERROR: cannot find $f"; exit 1; }; f=; set -e
# --- end runfiles.bash initialization v3 ---

# Materialize the CMake dependency prefix built by //cmake-deps:bundle into
# the workspace, so OpenROAD can be built with plain CMake:
#
#   bazelisk run //:cmake
#   cmake -DCMAKE_TOOLCHAIN_FILE=deps/toolchain.cmake -B build .
#   cmake --build build -j
#
# The destination is wiped and recreated on every run; a stamp file guards
# against deleting a directory this script did not create.

BUNDLE="$(rlocation openroad/cmake-deps/bundle)"
if [ -z "$BUNDLE" ] || [ ! -d "$BUNDLE" ]; then
    echo >&2 "ERROR: bundle runfiles directory not found"
    exit 1
fi

# :? guards against BUILD_WORKSPACE_DIRECTORY being unset or empty (e.g.
# the script run directly instead of via bazel run), where the default
# would otherwise resolve to /deps.
DEST="${1:-${BUILD_WORKSPACE_DIRECTORY:?not set; run this via: bazelisk run //:cmake}/deps}"
STAMP="$DEST/.openroad-deps-stamp"

if [ -e "$DEST" ] && [ ! -e "$STAMP" ]; then
    echo >&2 "ERROR: $DEST exists but was not created by this tool" \
             "(missing $STAMP); refusing to delete it."
    exit 1
fi

rm -rf "$DEST"
mkdir -p "$DEST"

# "$BUNDLE/." resolves the runfiles symlink to the bundle tree itself;
# the copy then preserves the relative symlinks the assembler creates to
# recreate the multiplexed LLVM tool names (they resolve inside the tree,
# so deps/ stays self-contained).
cp -r "$BUNDLE/." "$DEST/"
chmod -R u+w "$DEST"
touch "$STAMP"

ABS_DEST="$(realpath "$DEST")"
echo ""
echo "CMake dependency prefix ready: $ABS_DEST"
echo ""
echo "Build OpenROAD with plain CMake:"
echo ""
echo "  cmake -DCMAKE_TOOLCHAIN_FILE=$ABS_DEST/toolchain.cmake -B build ."
echo "  cmake --build build -j\$(nproc)"
echo ""
echo "Run the result with the bundled Tcl and Python runtimes:"
echo ""
echo "  export TCL_LIBRARY=$ABS_DEST/lib/tcl9.0"
echo "  export PYTHONHOME=$ABS_DEST/python"
echo "  ./build/src/openroad"
echo ""
echo "Host requirements: cmake >= 3.16, ninja or make, git, bash."
echo "Linux x86_64 only; the GUI is not part of this prefix."
