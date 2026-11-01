# OpenROAD Build Pitfalls

## Building

Bazel is the only build system.

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
