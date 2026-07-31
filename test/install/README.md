# Install smoke test

Verifies that the binary produced by `//packaging:tarfile` starts correctly
after extraction (Tcl initialisation, runfiles resolution, etc.).

## Running

```sh
bazelisk test //test/install/...
```

## What it checks

1. Extracts the packaging tarball into a temporary directory (same layout
   that `bazelisk run //packaging:install` produces).
2. Launches the binary with a trivial Tcl script (`puts "install_test_ok"`).
3. Asserts the expected output appears, proving Tcl initialised and the
   interpreter is functional.
4. Runs the real `bazel/install.sh` inside a self-contained fake workspace
   (fake `bazel info bazel-bin`, a minimal tarball, synthetic desktop/icon
   sources) to verify its cleanup logic: stale binary runfiles are removed on
   re-install, the GUI launcher + icon are installed and refreshed for a GUI
   build, and the stale launcher + icon are removed when a later install is
   not a GUI build.

## Background

The installed binary relies on Bazel runfiles for Tcl library files
(`init.tcl`, tclreadline, etc.).  These live under
`openroad.runfiles/_main/bazel/tcl_resources_dir/`.  If the runfiles
tree is damaged during packaging or installation the binary fails with
`application-specific initialization failed:` (GitHub issue #10115).
