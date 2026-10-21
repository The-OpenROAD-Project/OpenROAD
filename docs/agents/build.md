# OpenROAD Build Pitfalls

## Building

Bazel is the build system. CMake is deprecated and will be removed on Nov. 1, 2026.

```bash
bazelisk build //:openroad   # the binary is bazel-bin/openroad
etc/Build.sh                 # wrapper script, also builds with Bazel
```

## Debug vs Release Build Behavior Differences

The Bazel default is `-c opt` (see `.bazelrc`).
Use `bazelisk build -c dbg //:openroad` to reproduce a failure that only shows in a Debug build.
`--config=release` only stamps the version and implies `opt`; it is not a Debug/Release switch.

When tests pass locally (Debug) but fail in CI (Release), check for:
- **Uninitialized members**: Debug zero-initializes memory; Release leaves garbage
- **nullptr through custom hashers**: e.g., `PinIdHash::operator()(nullptr)` segfaults in Release only
- **Subnormal float values**: Uninitialized floats may be zero in Debug but subnormal in Release

## Shared Library RPATH vs LD_LIBRARY_PATH (CMake builds only, deprecated)

This applies to CMake builds only.
In the Bazel build, `kepler-formal` is a `dev_dependency` in `MODULE.bazel` that Bazel builds from source.

External tool binaries (e.g., kepler-formal) have RPATH baked in pointing to the original install location. After rebuilding a dependency, you must either `cmake --install` to the RPATH location or use `LD_PRELOAD`/`patchelf`. Setting `LD_LIBRARY_PATH` alone may be insufficient if RPATH takes precedence.
