# libxml2 stub — boarding up a broken window

The pre-built LLVM toolchain ships `ld.lld` dynamically linked against
`libxml2.so.2`, but the only code path that uses it is Windows COFF manifest
merging — dead code on Linux.  Rather than tolerate the warning and the
implicit host dependency, we board it up with a stub shared library that
satisfies the linker but `abort()`s if ever called.

Every link action without this fix prints:

```
ld.lld: /lib/x86_64-linux-gnu/libxml2.so.2: no version information available
```

The stub is compiled into the `@llvm_prebuilt` external archive's `lib/`
directory via `patch_cmds` in `MODULE.bazel`, where `ld.lld`'s
`RUNPATH=$ORIGIN/../lib` picks it up instead of the system library.

## Files

| File | Purpose |
|---|---|
| `libxml2_stub.c` | Canonical C source (documentation / maintenance copy) |
| `libxml2_stub.ver` | GNU linker version script with `LIBXML2_2.4.30` / `2.6.0` tags |

The actual compilation is inlined in `MODULE.bazel` `patch_cmds` because
`patch_cmds` cannot reference workspace files.

## This is temporary

Bazel's toolchain ecosystem is rapidly improving.  Delete this hack when any
of the following happen:

- The LLVM release ships `ld.lld` without the libxml2 dependency
- The `toolchains_llvm` BCR module bundles its own fix or gains `patch_cmds`
  support
- We switch to a toolchain that doesn't have this issue
