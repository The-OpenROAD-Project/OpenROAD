#!/usr/bin/env bash
set -e

# Install binary and runfiles from bazel build

BAZEL_BIN=$(cd "$BUILD_WORKSPACE_DIRECTORY"; bazel info bazel-bin)
TARFILE="$BAZEL_BIN/packaging/openroad.tar"

# The desktop entry is built into bazel-bin only for GUI builds
# --//:platform=gui
DESKTOP_SRC="$BAZEL_BIN/src/gui/openroad.desktop"
ICON_SRC="$BUILD_WORKSPACE_DIRECTORY/src/gui/resources/icon.png"

INSTALL_DESKTOP=0
if [ "${OPENROAD_INSTALL_GUI:-0}" = "1" ]; then
    INSTALL_DESKTOP=1
fi

DEST_DIR=${1:-${BUILD_WORKSPACE_DIRECTORY}/../install/OpenROAD}

mkdir -p "$DEST_DIR/bin"

# Remove previous OpenROAD install artifacts.
rm -f "$DEST_DIR/bin/openroad"
rm -f "$DEST_DIR/bin/openroad.repo_mapping"
rm -f "$DEST_DIR/bin/openroad.runfiles_manifest"
rm -rf "$DEST_DIR/bin/openroad.runfiles"

tar -xf "$TARFILE" -C "$DEST_DIR/bin"

# Remove useless files from pkg_tar from bazel
if [ -e "$DEST_DIR/bin/openroad.repo_mapping" ]; then
    chmod u+w "$DEST_DIR/bin/openroad.repo_mapping"
    rm -rf "$DEST_DIR/bin/openroad.repo_mapping"
fi

ABS_DEST="$(realpath "$DEST_DIR")"
echo "OpenROAD binary installed to $ABS_DEST"

# Remove any previously installed desktop entry and icon before (re)installing,
# mirroring the binary artifact cleanup above. This runs unconditionally so a
# prior GUI install is cleaned up even when the current install is not a GUI
# build, avoiding a stale launcher/icon left pointing at an old prefix.
APPS_DIR="${HOME}/.local/share/applications"
rm -f "$ABS_DEST/share/openroad/gui/icon.png"
rm -f "$APPS_DIR/openroad.desktop"

# Install the desktop entry (menu launcher + icon) if Bazel built it, i.e.
# this is a GUI build.
if [ "$INSTALL_DESKTOP" = "1" ] && [ -f "$DESKTOP_SRC" ]; then
    GUI_SHARE="$ABS_DEST/share/openroad/gui"
    mkdir -p "$GUI_SHARE"
    cp -f "$ICON_SRC" "$GUI_SHARE/icon.png"

    mkdir -p "$APPS_DIR"
    cp -f "$DESKTOP_SRC" "$APPS_DIR/openroad.desktop"
    # Fill in the install prefix for the icon, and point Exec at the absolute
    # binary so the launcher works without openroad being on PATH.
    sed -i \
        -e "s|@OPENROAD_PREFIX@|$ABS_DEST|g" \
        -e "s|^Exec=openroad |Exec=$ABS_DEST/bin/openroad |" \
        "$APPS_DIR/openroad.desktop"
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$APPS_DIR" >/dev/null 2>&1 || true
    fi
    echo "OpenROAD desktop entry installed to $APPS_DIR/openroad.desktop"
fi
