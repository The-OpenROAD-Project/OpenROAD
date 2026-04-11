# Bazel visibility test for downstream consumers

## Problem

OpenROAD's `BUILD.bazel` restricted several targets to
`//:__subpackages__` visibility:

- `openroad_lib` (cc_library)
- `ord` (cc_library — public headers)
- `error_swig`, `error_swig-py`, `options_swig` (filegroups — SWIG `.i` files)

In Bazel's visibility model, `//:__subpackages__` means "packages within
the `@@openroad` repository" — e.g. `//src/drt`, `//src/rsz`.  A
downstream bzlmod consumer like `bazel-orfs` lives in a separate
repository (`@@bazel-orfs//`), so Bazel rejects the dependency at
analysis time.

These same targets are unconditionally public via CMake
(`install(TARGETS ...)`).  The restriction only exists in the Bazel
build, and it only matters once someone consumes OpenROAD as a bzlmod
dependency rather than building it standalone — which is exactly what
`bazel-orfs` does when building OpenROAD from source instead of pulling
the docker image.

Every downstream consumer had to carry an `openroad-visibility.patch`
(5 lines, `s/__subpackages__/public/`).  Because `single_version_override`
patches are root-module-only in bzlmod, the patch has to be duplicated in
every workspace that transitively depends on OpenROAD: `bazel-orfs` root,
`bazel-orfs/orfs/`, `bazel-orfs/gallery/`, `test/downstream/`, and any
end-user project.

## Fix

PR #10099 changes the five targets to `//visibility:public`, eliminating
the patch.

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
