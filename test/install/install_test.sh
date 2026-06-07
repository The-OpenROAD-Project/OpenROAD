#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Smoke test: extract the packaging tarball and verify the installed
# binary can start with cleared runfiles env vars.
set -euo pipefail

# Resolve to absolute paths now: the script cd's into a temp dir below, after
# which these runfiles-relative args would no longer resolve.
TARFILE="$(realpath "$1")"
INSTALL_SH="$(realpath "$2")"

DEST_DIR=$(mktemp -d)
WORK_DIR=$(mktemp -d)
trap 'rm -rf "$DEST_DIR" "$WORK_DIR"' EXIT

cp "$TARFILE" "$DEST_DIR"
cd "$DEST_DIR"
tar -xf openroad.tar
rm -f openroad.tar

# Remove repo_mapping (same as install.sh)
if [ -e openroad.repo_mapping ]; then
    chmod u+w openroad.repo_mapping
    rm -rf openroad.repo_mapping
fi

# Clear Bazel runfiles env vars so the installed binary resolves its own
# runfiles tree (not the test runner's).
unset RUNFILES_DIR RUNFILES_MANIFEST_FILE TEST_SRCDIR

# Verify CLI startup and trivial Tcl evaluation works.
echo 'puts "install_test_ok"' > test_script.tcl
if OUTPUT=$(./openroad -no_init -no_splash -exit test_script.tcl 2>&1) && echo "$OUTPUT" | grep -q "install_test_ok"; then
    echo "PASS: installed binary CLI startup works"
else
    echo "FAIL: installed binary CLI startup failed or produced unexpected output"
    echo "Output was: $OUTPUT"
    exit 1
fi

# Verify Main.cc runfiles initialization: RUNFILES_DIR should be present even
# though it was cleared in this test process.
cat > runfiles_env_test.tcl <<'EOF'
if {![info exists ::env(RUNFILES_DIR)]} {
  puts "runfiles_env_missing"
  exit 1
}
if {![file isdirectory $::env(RUNFILES_DIR)]} {
  puts "runfiles_dir_not_found"
  exit 1
}
puts "runfiles_env_ok"
EOF

if OUTPUT=$(./openroad -no_init -no_splash -exit runfiles_env_test.tcl 2>&1) && echo "$OUTPUT" | grep -q "runfiles_env_ok"; then
    echo "PASS: RUNFILES_DIR initialized by openroad"
else
    echo "FAIL: RUNFILES_DIR was not initialized as expected"
    echo "Output was: $OUTPUT"
    exit 1
fi

# Verify runfiles resolution when launched via PATH from an unrelated working
# directory. argv[0] is then just "openroad" (no path component); the binary
# must resolve the runfiles tree next to the installed executable found on
# PATH, not $PWD/openroad.runfiles. WORK_DIR deliberately has no runfiles tree.
cp runfiles_env_test.tcl "$WORK_DIR"/
if OUTPUT=$(cd "$WORK_DIR" && PATH="$DEST_DIR:$PATH" openroad -no_init -no_splash -exit runfiles_env_test.tcl 2>&1) && echo "$OUTPUT" | grep -q "runfiles_env_ok"; then
    echo "PASS: RUNFILES_DIR initialized when launched via PATH from another dir"
else
    echo "FAIL: RUNFILES_DIR not initialized when launched via PATH from another dir"
    echo "Output was: $OUTPUT"
    exit 1
fi

# ---------------------------------------------------------------------------
# install.sh artifact cleanup test.
#
# Runs the real, unmodified bazel/install.sh inside a self-contained fake
# workspace (a fake `bazel info bazel-bin`, a minimal tarball, and synthetic
# desktop/icon sources) so no live bazel is needed. Verifies that install.sh:
#   1. removes stale binary runfiles before re-extracting (existing cleanup);
#   2. installs the GUI launcher + icon for a GUI build, with the
#      @OPENROAD_PREFIX@/Exec placeholders substituted;
#   3. refreshes (updates) those files on a subsequent GUI re-install;
#   4. removes the stale launcher + icon when a later install is NOT a GUI
#      build (the unconditional GUI cleanup).
# ---------------------------------------------------------------------------
SHIM_DIR=$(mktemp -d)     # holds the fake `bazel` on PATH
WS=$(mktemp -d)           # BUILD_WORKSPACE_DIRECTORY (icon source lives here)
BB=$(mktemp -d)           # fake bazel-bin (tarball + desktop entry live here)
PREFIX=$(mktemp -d)       # install destination (DEST_DIR)
FAKE_HOME=$(mktemp -d)    # controls APPS_DIR (~/.local/share/applications)
trap 'rm -rf "$DEST_DIR" "$WORK_DIR" "$SHIM_DIR" "$WS" "$BB" "$PREFIX" "$FAKE_HOME"' EXIT

# Fake `bazel`: install.sh only ever calls `bazel info bazel-bin`.
cat > "$SHIM_DIR/bazel" <<EOF
#!/usr/bin/env bash
[ "\$1" = "info" ] && [ "\$2" = "bazel-bin" ] && { echo "$BB"; exit 0; }
exit 1
EOF
chmod +x "$SHIM_DIR/bazel"

# Minimal tarball: install.sh just extracts it into PREFIX/bin; the binary
# itself is irrelevant to the cleanup logic, so a dummy file keeps this fast.
mkdir -p "$BB/packaging" "$BB/src/gui" "$WS/src/gui/resources"
TAR_SRC=$(mktemp -d)
echo "dummy" > "$TAR_SRC/openroad"
tar -cf "$BB/packaging/openroad.tar" -C "$TAR_SRC" .
rm -rf "$TAR_SRC"

# Synthetic icon + desktop template mirroring //src/gui:desktop_entry: the
# template carries the @OPENROAD_PREFIX@ placeholder and an "Exec=openroad ..."
# line, both rewritten by install.sh at install time.
echo "icon-v1" > "$WS/src/gui/resources/icon.png"
cat > "$BB/src/gui/openroad.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=OpenROAD
Exec=openroad -gui
Icon=@OPENROAD_PREFIX@/share/openroad/gui/icon.png
Categories=Development;Electronics
EOF

# Run the real install.sh; arg $1 controls OPENROAD_INSTALL_GUI.
run_install() {
    env PATH="$SHIM_DIR:$PATH" \
        HOME="$FAKE_HOME" \
        BUILD_WORKSPACE_DIRECTORY="$WS" \
        OPENROAD_INSTALL_GUI="$1" \
        bash "$INSTALL_SH" "$PREFIX" > /dev/null
}

APPS_DIR="$FAKE_HOME/.local/share/applications"
INSTALLED_DESKTOP="$APPS_DIR/openroad.desktop"
INSTALLED_ICON="$PREFIX/share/openroad/gui/icon.png"

# 1. GUI install: launcher + icon present, placeholders substituted.
run_install 1
if [ ! -f "$INSTALLED_DESKTOP" ] || [ ! -f "$INSTALLED_ICON" ]; then
    echo "FAIL: GUI install did not create desktop entry and/or icon"
    exit 1
fi
ABS_PREFIX="$(realpath "$PREFIX")"
if grep -q "@OPENROAD_PREFIX@" "$INSTALLED_DESKTOP"; then
    echo "FAIL: @OPENROAD_PREFIX@ placeholder not substituted in desktop entry"
    exit 1
fi
if ! grep -q "^Exec=$ABS_PREFIX/bin/openroad " "$INSTALLED_DESKTOP"; then
    echo "FAIL: Exec line not rewritten to absolute installed binary path"
    exit 1
fi
echo "PASS: GUI install created launcher + icon with substituted paths"

# 2. Binary cleanup: a stale runfiles tree must be removed on re-install.
mkdir -p "$PREFIX/bin/openroad.runfiles/stale"
touch "$PREFIX/bin/openroad.runfiles/stale/old_file"
run_install 1
if [ -e "$PREFIX/bin/openroad.runfiles/stale/old_file" ]; then
    echo "FAIL: stale binary runfiles not removed on re-install"
    exit 1
fi
echo "PASS: re-install removes stale binary runfiles"

# 3. GUI re-install with changed sources: files are refreshed in place.
echo "icon-v2" > "$WS/src/gui/resources/icon.png"
run_install 1
if ! grep -q "icon-v2" "$INSTALLED_ICON"; then
    echo "FAIL: GUI re-install did not update the installed icon"
    exit 1
fi
echo "PASS: GUI re-install updates previously installed artifacts"

# 4. Non-GUI install: stale launcher + icon from the prior GUI install must be
#    removed even though this install ships no desktop files.
run_install 0
if [ -f "$INSTALLED_DESKTOP" ]; then
    echo "FAIL: stale desktop entry not removed on non-GUI re-install"
    exit 1
fi
if [ -f "$INSTALLED_ICON" ]; then
    echo "FAIL: stale icon not removed on non-GUI re-install"
    exit 1
fi
echo "PASS: non-GUI re-install removes stale GUI artifacts"
