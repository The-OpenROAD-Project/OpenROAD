# OpenROAD Build Pitfalls

## Debug vs Release Build Behavior Differences

When tests pass locally (Debug) but fail in CI (Release), check for:
- **Uninitialized members**: Debug zero-initializes memory; Release leaves garbage
- **nullptr through custom hashers**: e.g., `PinIdHash::operator()(nullptr)` segfaults in Release only
- **Subnormal float values**: Uninitialized floats may be zero in Debug but subnormal in Release

## Shared Library RPATH vs LD_LIBRARY_PATH

External tool binaries (e.g., kepler-formal) have RPATH baked in pointing to the original install location. After rebuilding a dependency, you must either `cmake --install` to the RPATH location or use `LD_PRELOAD`/`patchelf`. Setting `LD_LIBRARY_PATH` alone may be insufficient if RPATH takes precedence.
