# Bazel visibility test for downstream consumers

## Why each target is public

### `openroad_lib` (cc_library) and `ord` (cc_library — headers)

These are the core C++ library and public headers. A downstream
Bazel consumer that wants to build a custom binary linking against
the OpenROAD library (rather than just running the prebuilt
`openroad` binary) must be able to depend on these targets.

### Why NOT the SWIG filegroups

`error_swig`, `error_swig-py`, and `options_swig` stay at
`//:__subpackages__`. They are referenced by internal subpackages
(e.g. `//src/gpl:swig` depends on `//:error_swig`), which already
satisfies the `__subpackages__` visibility check. No downstream
consumer references them directly.

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
