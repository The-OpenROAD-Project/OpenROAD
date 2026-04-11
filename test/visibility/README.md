# Bazel visibility test for downstream consumers

## Problem

OpenROAD's `BUILD.bazel` restricted several targets to
`//:__subpackages__` visibility:

- `openroad_lib` (cc_library)
- `ord` (cc_library — public headers)
- `error_swig`, `error_swig-py`, `options_swig` (filegroups — SWIG `.i` files)

This blocks any downstream Bazel module that depends on `@openroad` from
using these targets.  The CMake build exposes them unconditionally.

In practice, every downstream consumer (e.g. `bazel-orfs`) had to carry
an `openroad-visibility.patch` that changes the five targets to
`//visibility:public`.

## Fix

PR #10099 changes the five targets to `//visibility:public`.

## How to test

```bash
cd test/visibility
bazelisk build --nobuild :all
```

If any target is not publicly visible, Bazel fails at analysis time:

```
ERROR: target '@openroad//:openroad_lib' is not visible from
target '@@visibility-test//:openroad_lib'.
```

After the fix, the command succeeds (exit 0).

## Not run in CI

This test is a standalone Bazel workspace for manual verification only.
It is listed in the root `.bazelignore` so it does not interfere with the
main build.
