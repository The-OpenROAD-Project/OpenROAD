# Testing local changes with Bazel

First [install Baselisk](https://bazel.build/install/bazelisk), then you're ready for the main use-case of Bazel, which is to make modifications to OpenROAD and run fast local tests before creating a PR:

    bazelisk test --jobs=4 src/...

- `...` means everything below this folder, so use `src/gpl/...` to run a smaller set of tests.
- `--jobs=4` limits parallel builds to 4 cores, default is use all cores.

For more comprehensive testing locally, includes longer OpenROAD integration tests and some ORFS smoke tests, install either [podman](https://podman.io/), which works without root permissions or Docker, then run:

    bazelisk test ...

Note! You'll see `bazel` in examples and documentation as well as `bazelisk`. The latter is a wafer thin layer on top of `bazel` that reads in the `.bazelversion` file to decide which version of Bazel to use in bazel.

A word on expectations: Bazel is a valuable skill for the future. It is an example of a new generation of build tools(Buck2 is another example) that scales very well, are hermetic and have many features, such as artifacts. Unsurprisingly, this does mean that there is a lot to learn. The OpenROAD documentation makes no attempt at teaching Bazel. Bazel is a very wide topic and it can not be learned in a day or two with intense reading of a well defined document with a start and an end. It is probably best to start as a user running canned commands, such as above, but switch from mechanical repetition of canned command to being curious and following breadcrumbs of interest: read, search, engage with the community and use AI to learn.

## Running specific tests or tests below a folder

To list all tests use bazelisk query with [Bazel query language](https://bazel.build/query/language):

    bazelisk query 'kind(test, ...)'

List all tests below a folder containing `asap7`:

    $ bazelisk query   'filter("asap7", kind(".*_test rule", //src/pdn/...))'
    //src/pdn/test:asap7_M1_M3_followpins-tcl
    ...

Run specific test:

    baselisk test --test_output=errors //src/upf/test:levelshifter-tcl

To run or list tests below a folder:

    bazelisk test src/gpl/...

## Build configurations

Bazel build configuration is a big topic as it covers cross compilation as well as multiplatform support.

As a start, browse `.bazelrc` and also run:

    $ bazelisk build
    [wait until it starts building then hist ctrl-c, this warms up the cache with information about available configs]
    $ bazelisk config
    Available configurations:
    5e5e8a80a777eb91e67b2d19d33945262b2897b636da1007246cf68b6b6ec51d k8-fastbuild
    781deb76199c2d7f1a6c7d54da3ea8dad03e6956c04db58addba49068e4d3797 k8-fastbuild
    96deab75888eab9e42bcc5778aa824b18bb8ee745dfa1950a18536d75ee05e01 k8-opt-exec-ST-d57f47055a04 (exec)
    dd6f5325b352d3c63abe45b22d15914f75da7e2f140ace0017440dfc00f49114 k8-opt-exec-ST-d57f47055a04 (exec)
    f37096aa0a6acd138beca4ff4d66b677012cd1d0d54befaa35983993505dad60 fastbuild-noconfig
    f6ad5d5ec52c67510c6642c6d8df81fb134760a95c43d1f8e0369ad2c8964a81 k8-opt-exec-ST-6f5a6fb95be7 (exec)

Without offering any deeper insight some comments about what is shown above:

- Each configuration has a textual shorthand used in folder names followed by a full hash
- `fastbuild` is the default compromise between optimization and fast builds
- `exec` means host, appears with `ST` and an extra hash at the end
- `k8` always there, possibly referring to [K8](https://en.wikipedia.org/wiki/X86-64)

### LTO

The default Bazel build (`fastbuild`) does not enable Link Time Optimization
(LTO). `-c opt` enables optimization without LTO; `--config=opt` adds LTO
on top, which improves runtime by ~11% at the cost of a much longer
(single-threaded) link step. Use `--config=opt` for production binaries
shipped to end users; keep the default for the local edit-rebuild loop.
The CMake build has LTO on by default in Release mode -- see the
[CMake LTO option](Build.md#lto-options).

## Using OpenROAD as a dependency from another project

OpenROAD can be consumed as a Bazel module (`bazel_dep`) from another
project. The public API consists of two targets:

| Target | Description |
| --- | --- |
| `@openroad//:openroad` | The CLI binary |
| `@openroad//:openroad_py` | Python bindings for scripting |

All other targets (e.g. `openroad_lib`, internal libraries) are
restricted to OpenROAD's own subpackages and are not part of the
public API.

### Minimal MODULE.bazel for a downstream project

```starlark
module(name = "my-project")

bazel_dep(name = "openroad")
git_override(
    module_name = "openroad",
    commit = "<commit-hash>",
    init_submodules = True,
    remote = "https://github.com/The-OpenROAD-Project/OpenROAD.git",
)

# qt-bazel is not in BCR; git_override is root-module-only,
# so downstream consumers must repeat it.
bazel_dep(name = "qt-bazel")
git_override(
    module_name = "qt-bazel",
    commit = "df022f4ebaa4130713692fffd2f519d49e9d0b97",
    remote = "https://github.com/The-OpenROAD-Project/qt_bazel_prebuilts",
)
```

### Suggested: pin the C++ toolchain for reproducibility

OpenROAD uses [hermetic-llvm](https://github.com/hermeticbuild/hermetic-llvm)
(BCR module `llvm`) internally to lock the compiler version and ensure
reproducible builds across developers and CI: statically linked LLVM
binaries and a zero-sysroot cc_toolchain, so no host compiler, headers
or libraries are involved. Downstream consumers can use any
C++20-capable compiler, but pinning the same toolchain is recommended
to avoid compiler-specific issues:

```starlark
bazel_dep(name = "llvm", version = "0.8.14")

register_toolchains("@llvm//toolchain:all")
```

### Dev dependencies not leaked to consumers

The following are `dev_dependency` in OpenROAD and will not be forced
on downstream projects via MVS:

- rules_pkg — only needed for //:install
- `rules_verilator`, `verilator` — only needed for test/orfs simulation
- `llvm` (hermetic-llvm) toolchain registration

The downstream test at `test/downstream/` verifies these invariants.

## Build without testing

    bazelisk build :openroad

## Version stamping

By default, Bazel dev builds use a fixed placeholder version (`bazel-nostamp`) so that the build output is deterministic and fully cacheable across commits.

To embed the real git version (e.g. `26Q1-1486-g6fe48208e4`), use `--config=release`:

    bazelisk build --config=release :openroad
    ./bazel-bin/openroad -version

A release build reads the version with `git describe`, and gives `unknown` when
the tree has no git metadata. Set `OPENROAD_VERSION` to supply the version
directly:

    OPENROAD_VERSION=26Q1 bazelisk build --config=release :openroad

The Docker build needs this, because `.dockerignore` keeps `.git` out of the
build context. Pass the version with `--build-arg orVersion=<version>`.

`etc/Build.sh` always passes `--config=release` and forwards `OPENROAD_VERSION`,
so installs made through it (including ORFS `build_openroad.sh` and the Docker
build) carry the real version.

## Platforms

https://bazel.build/extending/platforms

Note that this builds a different configuration than is used during tests. Tests run with the `cfg=exec` configuraiton, whereas the above builds the `cfg=target` configuration. The TL;DR is that you are probably better of running a single test to test building so that you don't have to rebuild if you want to run tests after testing build.

## Can I force a rebuild?

You should never have to in Bazel as builds are robust, trust the caching...

That said:

    bazelisk clean

Or more forcefully:

    bazelisk clean --expunge

Or "nuke it from orbit":

    baselisk shutdown
    sudo pkill -9 java
    sudo rm -rf ~/.cache/bazel

## Run tests with [address sanetizers](https://github.com/google/sanitizers/wiki/addresssanitizer):

    bazelisk test --config=asan src/...

Example output:

```
[deleted]
Direct leak of 18191 byte(s) in 2525 object(s) allocated from:
    #0 0x5f8556654e04  (/home/oyvind/.cache/bazel/_bazel_oyvind/896cc02f64446168f604c13ad7b60f8b/execroot/_main/bazel-out/k8-opt-exec-ST-d57f47055a04/bin/external/org_swig/swig+0x37de04) (BuildId: f982b51b51338154ba961612c62b330f)
[deleted]
SUMMARY: AddressSanitizer: 27236 byte(s) leaked in 3801 allocation(s).
[deleted]
```

## Run tests with the [thread sanitizer](https://github.com/google/sanitizers/wiki/threadsanitizercppmanual):

    bazelisk test --config=tsan --test_tag_filters=-py src/...

Or to get an instrumented binary to run under ORFS:

    bazelisk build --config=tsan :openroad

The first `--config=tsan` invocation builds compiler-rt's tsan runtime from
source, so expect a few minutes before any OpenROAD source is compiled.

Instrumented code runs roughly 5-15x slower and uses far more memory, so
prefer the smallest design that reproduces the race. A full `src/...` run at
Bazel's default parallelism can exhaust RAM and get the build OOM-killed;
throttle it if that happens:

    bazelisk test --config=tsan --test_tag_filters=-py --jobs=16 --local_test_jobs=8 src/...

Adjust the runtime via `TSAN_OPTIONS`, e.g. to keep going past the first
report and get the second stack of a lock-order inversion:

    bazelisk test --config=tsan --test_tag_filters=-py --test_env=TSAN_OPTIONS="halt_on_error=0 second_deadlock_stack=1" src/...

`drt`, `gpl`, `grt` and `ant` parallelize with OpenMP. The `@openmp` runtime
is instrumented along with everything else under `--config=tsan`, but it is
not built with OpenMP's TSan annotations (`LIBOMP_TSAN_SUPPORT`), so races
reported inside `__kmp_*` frames are artifacts of the barrier implementation
rather than OpenROAD bugs. Confirm a finding by checking that both stacks land
in OpenROAD code.

### Known findings

A `--config=tsan --test_tag_filters=-py src/...` sweep reports 36 of 3980
tests failing, and the OpenMP artifacts above are almost all of it:

| Origin | Tests |
| --- | --- |
| `@openmp` `runtime/src/kmp_runtime.cpp`, `kmp_wait_release.h` | 33, across `drt`, `grt`, `ram`, `rcx`, `gpl` |
| `src/sta/graph/Graph.cc:1288` | 1, via `rmp` |
| `boost::asio` `scheduler.ipp:187` | 1, in `dst` |

Silencing the OpenMP group means building `@openmp` with
`LIBOMP_TSAN_SUPPORT`, which is a change to that module rather than something
this repo can pass as a flag. The remaining two are real: the `dst` one is a
test that destroys a stack `io_context` while its thread still runs it, and
the OpenSTA one is upstream.

`--test_tag_filters=-py` skips the Python tests; see "Sanitizers and the
Python extension modules" below for why.

## Run tests with the [undefined behavior sanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html):

    bazelisk test --config=ubsan --test_tag_filters=-py src/...

Or to get an instrumented binary to run under ORFS:

    bazelisk build --config=ubsan :openroad

UBSan is much cheaper than asan/tsan -- a small constant slowdown, no memory
overhead -- so it is the cheapest of the three to leave running over a real
design.

The config builds with `-fno-sanitize-recover=all`, so the first finding
aborts the process. That is deliberate: `-fsanitize=undefined` on its own only
prints a report and lets the process run on to exit 0, which would leave a
test suite green with the reports buried in the logs. To survey everything a
run would hit instead of stopping at the first, opt back into recovery:

    bazelisk test --config=ubsan --test_tag_filters=-py --copt=-fsanitize-recover=all src/...

Reports name the check that fired (e.g. `signed-integer-overflow`,
`misaligned-address`). An individual check can be switched off project-wide
with a copt, which is preferable to disabling the config wholesale:

    bazelisk test --config=ubsan --test_tag_filters=-py --copt=-fno-sanitize=vptr src/...

### Known findings

A `--config=ubsan --test_tag_filters=-py src/...` sweep is not clean yet. As of
this writing 36 of 3980 tests fail, in two groups.

Third-party UB reached through headers that are inlined into OpenROAD
translation units, 24 tests:

| Origin | Check | Tests |
| --- | --- | --- |
| `boost.geometry` `strategies/cartesian/intersection.hpp` | `undefined-behavior` | 19, via `pad::RDLRouter::isEdgeObstructed` |
| `coin-or-lemon` `lemon/network_simplex.h` | `signed-integer-overflow` | 5, in `cts` |

`--per_file_copt=.*external/.*@-fno-sanitize=all` above exempts third-party
*source* files, which is why abc, tcl and bliss no longer report. It cannot
exempt a third-party *header* compiled as part of our own translation unit --
the same limitation the `-w` per_file_copt has. Doing that needs an
`-fsanitize-ignorelist`, which has to be declared as a compile action input,
so it belongs in the toolchain rather than in this file.

OpenROAD's own UB, 11 tests, to be fixed separately:

| Site | Check | Tests |
| --- | --- | --- |
| `src/rcx/src/netRC.cpp:374`, `:1575`, `:1576` | `load of value` | 8 |
| `src/odb/include/odb/geom.h:373` | `signed-integer-overflow` | 2 |
| `src/grt/src/cugr/src/geo.h:111` | `signed-integer-overflow` | 1 |

## Sanitizers and the Python extension modules

`--config=tsan` and `--config=ubsan` cover the C++ and Tcl tests, which run
the instrumented `openroad` binary. They do **not** work for the `py`-tagged
tests, which import OpenROAD as a Python extension module:

    ImportError: _openroadpy.so: undefined symbol: __ubsan_handle_pointer_overflow_abort

Clang links a sanitizer runtime into executables but not into shared
libraries, assuming whoever loads the library provides the symbols. That holds
for `openroad`, which statically links the runtime; it fails for
`_openroadpy.so` / `_odb.so` / `_utl.so`, which the *uninstrumented* system
`python3` dlopens.

`-shared-libsan` is the mechanism for this case, but three things in
hermetic-llvm block it today:

1. only the static archives are staged into clang's resource directory, so the
   driver cannot find `libclang_rt.<san>.so` (asan stages both, ubsan and tsan
   stage static only);
2. the shared runtimes are built by `cc_shared_library` with sonames like
   `libubsan_standalone.shared.so`, so a consumer records a `DT_NEEDED` on a
   name that does not match the staged `libclang_rt.ubsan_standalone.so`;
3. the runtime arrives via a linkopt rather than a dep, so it lands in neither
   the test's runfiles nor its RPATH.

Fixing this belongs upstream in hermetic-llvm. Until then, skip them with
`--test_tag_filters=-py`; the Tcl tests cover the same C++ code paths as their
Python counterparts.

The Tcl tests do get instrumented. `test/regression.bzl` normally takes the
`openroad` binary from the exec configuration, to avoid building it twice when
it is also used as a build tool by bazel-orfs. The sanitizer configs only
instrument the target configuration, so under `//bazel:sanitizer_build` the
rule takes the binary from there instead. Only the binary under test moves;
swig, bison and the other exec-configuration tools stay uninstrumented, and
because exactly one of the two attributes is ever set `openroad` is still
built once.
## GPU build (`--config=gpu`)

`--config=gpu` compiles the Kokkos/CUDA backends of `gpl` (`src/gpl/src/gpu`)
and is the Bazel counterpart of the CMake `ENABLE_GPU` flow. It is opt-in and
non-hermetic: the CUDA toolkit, Kokkos and KokkosFFT come from the host, wrapped
as external repositories by `bazel/gpu/system_gpu.bzl`. Nothing in the default
build or in CI depends on them.

    bazelisk test --config=gpu //src/gpl/test/...

Install prefixes are read from the shell environment:

| Variable             | Default                        | Meaning                                              |
|----------------------|--------------------------------|------------------------------------------------------|
| `KOKKOS_ROOT`        | `/usr/local/kokkos-libcxx`     | Kokkos install (static, CUDA backend, see recipe)    |
| `KOKKOS_FFT_ROOT`    | `/usr/local/kokkos-fft-libcxx` | KokkosFFT install built against that Kokkos          |
| `OPENROAD_CUDA_PATH` | `/usr/local/cuda-12.8`         | Full CUDA toolkit (`bin/ptxas`, `nvvm/libdevice`)    |
| `OPENROAD_CUDA_ARCH` | unset                          | Only needed if it cannot be read from the Kokkos install |

A missing or unusable install never breaks the CPU build: the wrapped
repository becomes a stub, and only a `--config=gpu` build fails, at analysis
time, with a message that names the variable and the problem (prefix missing,
incomplete install, Kokkos built without CUDA, architecture mismatch).

### Why the CMake-flow Kokkos does not work here

The hermetic LLVM toolchain compiles against its bundled libc++ and links
against its own glibc-2.28 sysroot. A Kokkos built the usual way (nvcc or g++,
libstdc++, the host's glibc headers) fails to link: either with libstdc++/libc++
ABI mismatches (undefined `std::__1::...` symbols) or, on newer distributions,
with `undefined symbol: __isoc23_strtol` because the host headers redirect
`strtol` to symbols the sysroot's glibc stub does not export. Kokkos and
KokkosFFT therefore have to be built with the toolchain's own clang and header
set.

### Kokkos recipe

Run from an OpenROAD checkout after any `bazelisk build`, so the toolchain has
been fetched. The header directories below are the ones the Bazel
`cc_toolchain` uses.

```bash
OB=$(bazelisk info output_base)
EX=$OB/execroot/_main
case "$(uname -m)" in
  x86_64)  OUT=k8-opt;      TC=llvm-toolchain-minimal-linux-amd64 ;;
  aarch64) OUT=aarch64-opt; TC=llvm-toolchain-minimal-linux-arm64 ;;
esac
CLANGXX=$(ls -d $OB/external/*${TC}/bin/clang++)
GLIBC_H=$(ls -d $OB/external/llvm++glibc+glibc_headers_*/include)
KERNEL_H=$(ls -d $OB/external/llvm++kernel_headers+*/include)
LIBCXX_H=$EX/bazel-out/$OUT/bin/external/llvm++llvm+llvm-project/libcxx/libcxx_headers_include_search_directory
LIBCXXABI_H=$EX/bazel-out/$OUT/bin/external/llvm++llvm+llvm-project/libcxxabi/libcxxabi_headers_include_search_directory

CUDA_HOME=/usr/local/cuda-12.8          # match OPENROAD_CUDA_PATH
KOKKOS_ARCH=BLACKWELL120                # match your device, see below

HFLAGS="--sysroot=/dev/null -isystem $LIBCXX_H -isystem $LIBCXXABI_H -isystem $KERNEL_H -isystem $GLIBC_H"
CUFLAGS="--cuda-path=$CUDA_HOME -Wno-unknown-cuda-version -D_ALLOW_UNSUPPORTED_LIBCPP"

git clone https://github.com/kokkos/kokkos.git && cd kokkos   # 4.7 or newer
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local/kokkos-libcxx \
  -DCMAKE_CXX_COMPILER="$CLANGXX" \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_CXX_FLAGS="$HFLAGS $CUFLAGS -stdlib=libc++" \
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DCMAKE_CXX_ARCHIVE_CREATE="<CMAKE_AR> qc <TARGET> <OBJECTS>" \
  -DCMAKE_CXX_ARCHIVE_APPEND="<CMAKE_AR> q <TARGET> <OBJECTS>" \
  -DCMAKE_CXX_ARCHIVE_FINISH="<CMAKE_RANLIB> <TARGET>" \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DKokkos_ENABLE_SERIAL=ON \
  -DKokkos_ENABLE_CUDA=ON \
  -DKokkos_ENABLE_CUDA_CONSTEXPR=ON \
  -DKokkos_ENABLE_DEPRECATED_CODE_4=ON \
  -DKokkos_ARCH_${KOKKOS_ARCH}=ON \
  -DKokkos_ENABLE_TESTS=OFF -DKokkos_ENABLE_EXAMPLES=OFF -DKokkos_ENABLE_BENCHMARKS=OFF
cmake --build build -j && sudo cmake --install build
```

Each option that is easy to drop matters:

- `--sysroot=/dev/null` plus the four `-isystem` directories: the toolchain's
  header set, not the host's (the `__isoc23_*` failure above).
- `-D_ALLOW_UNSUPPORTED_LIBCPP`: CUDA's `host_defines.h` refuses libc++ on
  x86_64 otherwise.
- `-DCMAKE_POSITION_INDEPENDENT_CODE=ON`: the static archives end up inside
  OpenROAD's Python extension (`_gpl.so`); without PIC the final link fails
  with relocation errors.
- `-DKokkos_ENABLE_DEPRECATED_CODE_4=ON`: `gpl` uses `View::HostMirror`, which
  Kokkos 5 only provides behind this flag.
- `-DCMAKE_CXX_STANDARD=20`: must match `.bazelrc` (`-std=c++20`).
- `-DKokkos_ARCH_...`: exactly one NVIDIA architecture, and it must be the
  compute capability of the device the tests run on. `--config=gpu` reads it
  back from the install's `KokkosCore_config.h` and compiles `gpl`'s kernels for
  the same target. This is not a cosmetic choice: `sm_120` kernels on an
  `sm_121` device (`BLACKWELL120` vs `BLACKWELL121`, e.g. an RTX 5090 vs a GB10)
  run without any error and produce NaN placements.

`nvidia-smi --query-gpu=compute_cap --format=csv,noheader` prints the compute
capability (`12.0` -> `BLACKWELL120`, `12.1` -> `BLACKWELL121`). `sm_121`
needs CUDA 12.9 or newer; with CUDA 13 also set `OPENROAD_CUDA_PATH` to that
toolkit (its libcu++ headers live in `include/cccl/`, which `--config=gpu`
already adds to the search path).

### KokkosFFT recipe

KokkosFFT is header-only but must be configured against the Kokkos above with
the FFTW host backend enabled (`gpl` creates host-space plans). FFTW itself is
built from source by Bazel (the `fftw` module in `MODULE.bazel`), so no system
FFTW is needed at build time.

```bash
git clone https://github.com/kokkos/kokkos-fft.git && cd kokkos-fft
cmake -S . -B build \
  -DCMAKE_INSTALL_PREFIX=/usr/local/kokkos-fft-libcxx \
  -DCMAKE_CXX_COMPILER="$CLANGXX" \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_CXX_FLAGS="$HFLAGS $CUFLAGS -stdlib=libc++" \
  -DKokkos_ROOT=/usr/local/kokkos-libcxx \
  -DKokkosFFT_ENABLE_FFTW=ON \
  -DKokkosFFT_ENABLE_TESTS=OFF -DKokkosFFT_ENABLE_EXAMPLES=OFF -DKokkosFFT_ENABLE_BENCHMARKS=OFF
sudo cmake --install build
```

### Notes

- The runtime gate is the `ENABLE_GPU` environment variable, read by
  `gpl::gpuEnabled()`. It defaults to on when the GPU code is compiled in, so
  every regression test pins `ENABLE_GPU=0` on a GPU build to stay on its CPU
  golden logs (`test/regression.bzl`); the GPU-only tests
  (`region01_gpu`, `region01_gpu_asym`, `fft_gpu_test`, `wl_gpu_test`) pin it
  to 1, are tagged `gpu`, and are reported as SKIPPED by plain CPU wildcard runs.
- `--config=gpu` binaries link libcudart/libcufft by absolute path with an
  rpath into the toolkit; they are not relocatable to another machine.
- The ORFS flow targets under `test/orfs` run their stages as build actions,
  which carry no `ENABLE_GPU` pin, so under `--config=gpu` they place on the
  GPU. The GPU placer is not bit-identical to the CPU one and the gcd/asap7
  metadata rules are tuned for the CPU result, so `//test/orfs/...` is not
  expected to pass under `--config=gpu`; run it on the default build.
- After upgrading Kokkos, KokkosFFT or CUDA in place, run `bazelisk shutdown`
  (or change the corresponding environment variable) so the wrapped repository
  is refetched with the new file list.

## Testing an OpenROAD build with ORFS from within the OpenROAD folder

    OPENROAD_EXE=$(pwd)/bazel-out/k8-opt-exec-ST-d57f47055a04/bin/openroad make --dir ~/OpenROAD-flow-scripts/flow/ DESIGN_CONFIG=designs/asap7/gcd/config.mk clean_floorplan floorplan

`$(pwd)/bazel-out/k8-opt-exec-ST-d57f47055a04/bin/openroad` points to the `cfg=exec` optimized configuration which is Bazel builds to run tests on host with e.g. `bazelisk test ...`

## Testing debug build of OpenROAD with ORFS

Build OpenROAD in debug mode using a [workaround](https://github.com/The-OpenROAD-Project/OpenROAD/issues/7349):

    bazelisk build --cxxopt=-stdlib=libstdc++ --linkopt=-lstdc++ -c dbg :openroad

Run ORFS flow and use debugger as usual for ORFS:

    OPENROAD_EXE=$(pwd)/bazel-out/k8-dbg/bin/openroad make --dir ~/OpenROAD-flow-scripts/flow/ DESIGN_CONFIG=designs/asap7/gcd/config.mk clean_floorplan floorplan

## Profiling OpenROAD with an ORFS build and your favorite profiling tool

Build an optimized profile binary, using a [workaround](https://github.com/The-OpenROAD-Project/OpenROAD/issues/7349):

    bazelisk build --config=profile --cxxopt=-stdlib=libstdc++ --linkopt=-lstdc++ :openroad

`bazel-bin` points to the results of the most recent `bazelisk build`. If you are switching between various builds, the more robust alternative is to point `OPENROAD_EXE` to the specific build configuration you want in `bazel-out`.

Start an ORFS job that you want to profile:

    OPENROAD_EXE=$(pwd)/bazel-bin/openroad make --dir ~/OpenROAD-flow-scripts/flow/ DESIGN_CONFIG=designs/asap7/gcd/config.mk clean_floorplan floorplan

At this point, use your favorite preformance tool, such as the Linx Perf tool:

    perf top

Perhaps attach gdb and use ctrl-c from the command line? Use gdb with an IDE, emacs, or vim?

    $ gdb bazel-bin/openroad
    [deleted]
    (gdb) attach 578603
    Attaching to program: /home/oyvind/.cache/bazel/_bazel_oyvind/896cc02f64446168f604c13ad7b60f8b/execroot/_main/bazel-out/k8-fastbuild/bin/openroad, process 578603
    #7  0x00005efc3b72b865 in isPolygonCorner () at src/drt/src/gc/FlexGC_init.cpp:705
    705	  poly_set.get(polygons);
    (gdb) list
    700	bool isPolygonCorner(const frCoord x,
    701	                     const frCoord y,
    702	                     const gtl::polygon_90_set_data<frCoord>& poly_set)
    703	{
    704	  std::vector<gtl::polygon_90_with_holes_data<frCoord>> polygons;
    705	  poly_set.get(polygons);
    706	  for (const auto& polygon : polygons) {
    707	    for (const auto& pt : polygon) {
    708	      if (pt.x() == x && pt.y() == y) {
    709	        return true;

## Creating an ORFS issue with bazel-orfs targets using `//:deps`

Consider a failure in `//test/orfs/mock-array:MockArray_floorplan` as one can find if carefully searching the logs for `ERROR:` and looking for `target`:

    $ bazelisk test //test/orfs/mock-array:MockArray_test
    [deleted]
    ERROR: /tmp/workspace/OpenROAD-Public_PR-7619-head/test/orfs/mock-array/BUILD:116:10: Action test/orfs/mock-array/results/asap7/MockArray/base/2_floorplan.odb failed: (Exit 2): bash failed: error executing Action command (from target //test/orfs/mock-array:MockArray_floorplan) /bin/bash -c ... (remaining 5 arguments skipped)
    [deleted]
    [ERROR MPL-0040] Failed on cluster root
    Error: macro_place.tcl, 5 MPL-0040
    [deleted]
    //test/orfs/mock-array:MockArray_test                           FAILED TO BUILD

To create an ORFS `make issue`, follow these steps:

    bazelisk run //:deps -- //test/orfs/mock-array:MockArray_floorplan

- In Bazel `//test/orfs/mock-array:MockArray_floorplan` failed and will leave behind no files, unless one uses `--sandbox_debug`
- bazel-orfs provides a `//:deps` wrapper that builds only the `deps` output group (cheap config/template operations) and deploys the dependencies for running `make do-floorplan`. The same works for any stage: synth, place, cts, grt, route or final.
- Files are placed in `tmp/test/orfs/mock-array/MockArray_floorplan_deps/` with a `make` script that is very nearly the same as `make DESIGN_CONFIG=...` with ORFS

First run `do-floorplan` until the failure, notice that the `do-` prefix is used to disable the dependency checking in ORFS as bazel-orfs handles dependencies:

    tmp/test/orfs/mock-array/MockArray_floorplan_deps/make do-floorplan

Now create an issue for e.g. `macro_place.tcl`:

    tmp/test/orfs/mock-array/MockArray_floorplan_deps/make macro_place_issue

The generated file is placed into the `_main` subfolder:

    tmp/test/orfs/mock-array/MockArray_floorplan_deps/_main/macro_place_MockArray_asap7_base_2025-06-19_21-50.tar.gz

## Creating an ORFS issue with bazel-orfs targets using `--sandbox_debug`

Hermeticity in Bazel requires some extra steps when debugging failures. If the action fails, then `--sandbox_debug` can be used. If the action succeeds or it is cached, `--sandbox_debug` does nothing.

If you have a failure in `//test/orfs/mock-array:MockArray_floorplan`, look for `ERROR:` and looking for `target`, find the error:

    $ bazelisk test //test/orfs/mock-array:MockArray_test
    [deleted]
    ERROR: /tmp/workspace/OpenROAD-Public_PR-7619-head/test/orfs/mock-array/BUILD:116:10: Action test/orfs/mock-array/results/asap7/MockArray/base/2_floorplan.odb failed: (Exit 2): bash failed: error executing Action command (from target //test/orfs/mock-array:MockArray_floorplan) /bin/bash -c ... (remaining 5 arguments skipped)
    [deleted]
    [ERROR MPL-0040] Failed on cluster root
    Error: macro_place.tcl, 5 MPL-0040
    [deleted]
    //test/orfs/mock-array:MockArray_test                           FAILED TO BUILD

Use `--sandbox_debug` to keep the files around after failure:

    bazelisk build //test/orfs/mock-array:MockArray_floorplan --sandbox_debug

Scan the log for setting up the shell and enviornment variables without linux-sandbox.

    (cd /home/oyvind/.cache/bazel/_bazel_oyvind/896cc02f64446168f604c13ad7b60f8b/sandbox/linux-sandbox/8901/execroot/_main && \
    exec env - \
        DESIGN_CONFIG=bazel-out/k8-fastbuild/bin/test/orfs/mock-array/results/asap7/MockArray/base/2_floorplan.mk \
        [deleted]
    /home/oyvind/.cache/bazel/_bazel_oyvind/install/772f324362dbeab9bc869b8fb3248094/linux-sandbox -t 15 -w /dev/shm -w /home/oyvind/.cache/bazel/_bazel_oyvind/
    [deleted]
    /mock-array/reports/asap7/MockArray/base/2_floorplan_final.rpt && external/bazel-orfs++orfs_repositories+docker_orfs/usr/bin/make $@' '' --file external/bazel-orfs++orfs_repositories+docker_orfs/OpenROAD-flow-scripts/flow/Makefile do-floorplan)

Do a bit of suregery to remove the `linux-sandbox` and `exec env -` part, which can be a bit tempremental, to launch a bash shell. This leaves you with a) changing directory b) setting up environment variables c) launching bash shell:

    (cd /home/oyvind/.cache/bazel/_bazel_oyvind/896cc02f64446168f604c13ad7b60f8b/sandbox/linux-sandbox/8901/execroot/_main && \
        DESIGN_CONFIG=bazel-out/k8-fastbuild/bin/test/orfs/mock-array/results/asap7/MockArray/base/2_floorplan.mk \
        [deleted]
    bash)

Now run `make issue` as usual:

    $ make --file external/bazel-orfs++orfs_repositories+docker_orfs/OpenROAD-flow-scripts/flow/Makefile macro_place_issue
    Archiving issue to macro_place_MockArray_asap7_base_2025-06-20_12-57.tar.gz
    Using pigz to compress tar file

## Some OpenROAD and OpenSTA Bazel Specifics

Bazel distinguishes between *host* (`cfg=exec`) and *target* (`cfg=target`) configurations, a concept that becomes important when cross-compilation or tool usage is involved.

In the OpenROAD Bazel build:

- `bazelisk build ...` builds all targets in the **target configuration** (`cfg=target`), assuming you're building for deployment or installation.
- `bazelisk test ...`, on the other hand, uses OpenROAD and OpenSTA **as host tools**, meaning they are built and run in the **execution configuration** (`cfg=exec`), often to run tests or launch `bazel-orfs` builds.

### ⚠️ Avoiding Redundant Builds

By default, `bazel test` would:
1. First build test dependencies in the **target** configuration.
2. Then build tools like OpenROAD/OpenSTA again in the **host** configuration to actually run the tests.

This causes unnecessary duplication.

To avoid this, `.bazelrc` includes the following to build only the tests:

    test --build_tests_only

## Using the OpenROAD project Bazel artifact server to download pre-built results

A single read only artifact server is configured in `.bazelrc` OpenROAD hosted projects.

This is a read only Bazel artifact server for anonymous access and is normally only updated by OpenROAD CI, though team OpenROAD team members can also update it directly.

## OpenROAD team member and CI - configuring write access to artifact server

If you only have a single Google account that you use for Google Cloud locally, you can use
`--google_default_credentials`.

If you are use multiple google accounts, using the default credentials can be cumbersome when
switching between projects. To avoid this, you can use the `--credential_helper` option
instead, and pass a script that fetches credentials for the account you want to use. This
account needs to have logged in using `gcloud auth login` and have access to the bucket
specified.

`.bazelrc` is under git version control and it will try to read in [user.bazelrc](https://bazel.build/configure/best-practices#bazelrc-file), which is
not under git version control, which means that for git checkout or rebase operations will
ignore the user configuration in `user.bazelrc`.

Copy the snippet below into `user.bazelrc` and specify your username by modifying `# user: myname@openroad.tools`:

    # user: myname@openroad.tools
    build --credential_helper=*.googleapis.com=%workspace%/etc/cred_helper.py

`cred_helper.py` will parse `user.bazelrc` and look for the username in the comment.

To test, run:

    $ ./cred_helper.py test
    Running: gcloud auth print-access-token oyvind@openroad.tools
    {
      "kind": "storage#testIamPermissionsResponse",
      "permissions": [
        "storage.buckets.get",
        "storage.objects.create"
      ]
    }

> **Note:** To test the credential helper, make sure to restart Bazel to avoid using a previous
cached authorization:

    bazel shutdown
    bazel build BoomTile_final_scripts

To gain write access to the https://storage.googleapis.com/megaboom-bazel-artifacts bucket,
reach out to Tom Spyrou, Precision Innovations (https://www.linkedin.com/in/tomspyrou/).

## Bisecting OpenSTA or OpenROAD with Bazel

Bisecting OpenROAD or OpenSTA requires finding a good and a bad commit. Normally in bisection, origin/master is bad, but finding a good commit is trickier because most commits are there solely to preserve review history and have not run through any extensive testing.

Fortunately, OpenSTA is a submodule in OpenROAD that is tested before it is updated, so all the submodule commits in OpenROAD of OpenSTA are known to be of good quality. Similarly for OpenROAD and ORFS.

A git/bash incantation will list the commit hashes of the src/sta submodule:

    $ git log --pretty=format:'%h' -- src/sta | while read commit; do  git show $commit src/sta| grep "Subproject commit" | awk '{print $3}'; done | head -n 10
    5ee1a315141d1c799a0b2532e90ddccf52ddee95
    3bff2d218c20adb867fcb3d8ae236f5da9928bed
    3bff2d218c20adb867fcb3d8ae236f5da9928bed
    5ee1a315141d1c799a0b2532e90ddccf52ddee95
    f21d4a3878e2531e3af4930818d9b5968aad9416
    3bff2d218c20adb867fcb3d8ae236f5da9928bed
    522fc9563f25728f456bf86c2eb665c60d823e74
    f21d4a3878e2531e3af4930818d9b5968aad9416
    fa0cdd65290843e4e5cbe39d0bb9f2a63d580d1f
    522fc9563f25728f456bf86c2eb665c60d823e74

To build OpenSTA, use master of the https://github.com/The-OpenROAD-Project/OpenSTA fork, because it contains Bazel build files:

    bazelisk build src/sta:opensta -c opt

Now start the bisection as usual with a bad and good commit from the above list:

    $ cd src/sta
    $ git bisect start origin/master 6e95d93a44f7c46bb572933f5e2f8a624135820b
    HEAD is now at 6e95d93a Merge remote-tracking branch 'parallax/master'
    Bisecting: 55 revisions left to test after this (roughly 6 steps)
    [03d2a48f462105a39b5850b8f45d6c5db16fd5f0] misc

Use `git bisect --skip` if the version does not build or otherwise should not be tested.

OpenSTA has an additional challenge in that only the https://github.com/The-OpenROAD-Project/OpenSTA fork has the Bazel BUILD file. To bisect the https://github.com/parallaxsw/OpenSTA branch, check out the branch you want, then check out BUILD from the fork and do a `git reset HEAD`. This will leave BUILD as a local file, because it is not in the upstream repository and bisection can be done on the upstream master branch.

## Testing the GUI with gcd on a pull request by number

To test a PR with the GUI on gcd, run:

```
    $ git fetch origin pull/7856/head
    $ git checkout FETCH_HEAD
    $ bazelisk run test/orfs/gcd:gcd_final gui_final
```

This will:

- fetch and checkout pull request 7856
- build OpenROAD
- run bazel-orfs flow on gcd
- set up ORFS project in `tmp/test/orfs/gcd/gcd_final/`
- launch the GUI opening gui_final gcd

`bazelisk run test/orfs/gcd:gcd_final` run alone would set up the project. Additional arguments are forwarded to the `tmp/test/orfs/gcd/gcd_final/make` script.

## Hacking ORFS with `//test/orfs/gcd:gcd_test` test case

First set up a local work folder with all dependencies for the step that you want to work on:

    bazelisk run //:deps -- //test/orfs/gcd:gcd_floorplan

Now run make directly with the work folder, but be sure to use the `do-` targets that side-step ORFS make dependency checking:

    make --file ~/OpenROAD-flow-scripts/flow/Makefile --dir tmp/test/orfs/gcd/gcd_floorplan_deps/_main DESIGN_CONFIG=config.mk do-floorplan

## Whittling down .odb files

Global place can take hours to run and to debug an error, the test case has to be whittled down to minutes, or it is probably intractable.

Consider an error such as:

    [ERROR GPL-0305] RePlAce diverged during gradient descent calculation, resulting in an invalid step length (Inf or NaN). This is often caused by numerical instability or high placement density. Consider reducing placement density to potentially resolve the issue.

First set up a folder with all the dependencies to run global placement:

    bazelisk run //:deps -- //test/orfs/gcd:gcd_place

Drop into a shell that has the build environment set up:

    $ tmp/test/orfs/gcd/gcd_place_deps/make bash
    Makefile Environment  tmp/test/orfs/gcd/gcd_place_deps/_main

Run up to the failing stage and stop with ctrl-c on the step that you want to run the whittling down on:

    make --file=$FLOW_HOME/Makefile do-place

Now run the whittler with stock `python3` — no extra packages needed beyond
the standard library. You are responsible for having `openroad` on your
`PATH` first (e.g. after `bazelisk run //:install` and `source env.sh` in
an ORFS checkout):

    python3 etc/whittle.py --error_string GPL-0305 --base_db_path 3_2_place_iop.odb --use_stdout --exit_early_on_error --step "make --file=$FLOW_HOME/Makefile do-3_3_place_gp"

This should eventually leave you with a whittled down .odb file. Copy the whittled down .odb file into the correct place for 3_2_place_iop.odb, then create a bug report:

    tmp/test/orfs/gcd/gcd_place_deps/make global_place_issue

### Monitoring progress

whittle.py prints `[whittle]` status lines showing the current phase,
element counts, .odb file size, and elapsed time.  After a step runs for
more than 5 minutes, whittle.py also shows the last 10 lines of the
step's log output so you can tell what the step is doing.

If the .odb size is not shrinking after 20+ steps, try different
parameters (`--persistence 1` for a coarser first pass, or a higher
`--multiplier`).  If each step takes more than 10 minutes, check that
`--error_string` is specific enough (avoid generic strings like "ERROR").

### Using Claude with whittle.py

Point Claude at a GitHub issue that has an attached tarball artifact
(from `make *_issue`).  Claude can download the artifact, reproduce the
bug with the latest OpenROAD, run whittle.py, and upload a smaller
test case.

| Scenario | Recommended flags |
| --- | --- |
| Global placement bugs | `--error_string GPL-XXXX --persistence 3 --multiplier 2` |
| Fast initial reduction | `--persistence 1` first, then increase |
| Large designs (>100K insts) | Start with `--timeout 600` |

See the `/triage-issue` Claude skill in `.claude/skills/triage-issue/`
for the full step-by-step workflow.
