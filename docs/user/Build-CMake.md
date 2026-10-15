# Building with CMake (deprecated)

```{warning}
**The CMake build is deprecated and will be removed on Nov. 1, 2026.**
The CMake configure step prints the same warning.
New users and new setups must use [Bazel](Build.md#build-and-install-with-bazel) instead.
The instructions on this page are only for existing CMake-based setups.
```

Each section names the Bazel equivalent.
Clone the repository as described in [Installing OpenROAD](Build.md#clone-repository) first.

The default build type is `RELEASE` to compile optimized code.
The resulting executable is `build/bin/openroad`.
Bazel: the executable is `bazel-bin/openroad`.

Optional CMake variables passed as `-D<var>=<value>` arguments to CMake are shown below.

| CMake argument         | Value                     | Bazel equivalent                           |
|------------------------|---------------------------|--------------------------------------------|
| `CMAKE_BUILD_TYPE`     | DEBUG, RELEASE            | `-c dbg`, `-c opt`                         |
| `CMAKE_CXX_FLAGS`      | Additional compiler flags | `--cxxopt=<flag>`                          |
| `TCL_LIBRARY`          | Path to Tcl library       | Not needed                                 |
| `TCL_HEADER`           | Path to `tcl.h`           | Not needed                                 |
| `ZLIB_ROOT`            | Path to `zlib`            | Not needed                                 |
| `CMAKE_INSTALL_PREFIX` | Path to install binary    | `//:install -- <dir>`                      |

```{note}
Every `./etc/Build.sh` run writes an `openroad_build.log` file in the build directory.
A direct `cmake` run does not.
In case of filing issues,
it can be uploaded in the "Relevant log output" section of OpenROAD
[issue forms](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/issues/new/choose).
```

## Only for macOS Setup

On macOS, it is recommended to use a Python virtual environment to isolate dependencies and avoid system conflicts.

1. Create a virtual environment in the OpenROAD directory:
``` shell
python3 -m venv .venv
```

2. Activate the virtual environment:
``` shell
source .venv/bin/activate
```

3. With virtual environment activated, run without `sudo`:
``` shell
./etc/DependencyInstaller.sh -cmake -base
./etc/DependencyInstaller.sh -cmake -common -local
```

Bazel: on macOS, `./etc/DependencyInstaller.sh -bazel` installs `bazelisk`.
See [Prerequisites](Build.md#prerequisites).

## Install Dependencies

We recommend using the `setup.sh` script located in the [OpenROAD-flow-scripts](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts) repository to install all dependencies. `setup.sh` encapsulates the calls to `DependencyInstaller.sh` and ensures the entire flow environment is configured correctly.

Alternatively, if you are building OpenROAD standalone, you may use our helper script.
The `-cmake` flag selects the dependencies of the CMake build.
Without it, `DependencyInstaller.sh` installs the Bazel dependencies.

``` shell
sudo ./etc/DependencyInstaller.sh -cmake -base
./etc/DependencyInstaller.sh -cmake -common -local
```

```{warning}
`sudo ./etc/DependencyInstaller.sh -cmake [-all|-common]` defaults to
installing packages on /usr/local.
To avoid this behavior use -local flag or -prefix <PATH> argument.
```

Bazel: `sudo ./etc/DependencyInstaller.sh -bazel`.

## Build OpenROAD

`./etc/Build.sh` builds with Bazel by default; the `-cmake-build` flag selects
the deprecated CMake build used in the rest of this page.

To build with the default options in release mode:

``` shell
./etc/Build.sh -cmake-build
```

Bazel: `bazelisk build :openroad`, or see [Using `etc/Build.sh`](Build.md#using-etcbuildsh).

These `Build.sh` flags only apply to the CMake build and have no effect on the default Bazel build:
`-cmake=`, `-no-tests`, `-no-warnings`, `-coverage` and `-build-man`.
The `-clean` and `-dir` flags only act on the `build/` directory.

### Custom Library Path

To build with debug option enabled and if the Tcl library is not on the default path.

``` shell
./etc/Build.sh -cmake-build -cmake="-DCMAKE_BUILD_TYPE=DEBUG -DTCL_LIBRARY=/path/to/tcl/lib"
```

Bazel: `bazelisk build -c dbg :openroad`.
Bazel does not need the Tcl library path.

### Enable `manpages`

To build the `manpages`:
``` shell
./etc/Build.sh -cmake-build -build-man
```

Bazel: `bazelisk build //docs:man_pages`.

### LTO Options

By default, OpenROAD is built with link time optimizations enabled.
This adds about 1 minute to compile times and improves the runtime
by about 11%. If you would like to disable LTO pass
`-DLINK_TIME_OPTIMIZATION=OFF` when generating a build.

Bazel: `--config=opt` enables ThinLTO. See [Bazel](Bazel.md#lto).

### Build with Address Sanitizer

To enable building with Address Sanitizer, use the argument `-DASAN=ON`.
Setting the `ASAN` variable to `ON` adds necessary compile and link options
for using Address Sanitizer.

```{note}
Address Sanitizer adds instrumentation for detecting memory errors.
Enabling this option will cause OpenROAD to run slower and consume more RAM.
```

Bazel: `bazelisk test --config=asan ...`. See [Bazel](Bazel.md).

### GPU Build

To build the opt-in GPU placement backends, pass `-DENABLE_GPU=ON`, or `-gpu` to `./etc/Build.sh -cmake-build`.
The CMake `ENABLE_GPU` flow uses the Kokkos and KokkosFFT
installations selected by its CMake configuration. That Kokkos must include
the Serial backend (`-DKokkos_ENABLE_SERIAL=ON`), which Kokkos's CMake turns
off by default once OpenMP is enabled; configuration stops with a message
otherwise.

Bazel: `--config=gpu`. See the GPU build section in [Bazel](Bazel.md).

### System wide OpenROAD Install

```{warning}
Only use the following instructions if you know what you are doing.
```

``` shell
sudo make install
```

The default install directory is `/usr/local`.
To install in a different directory with CMake use:

``` shell
./etc/Build.sh -cmake-build -cmake="-DCMAKE_INSTALL_PREFIX=<prefix_path>"
```

Alternatively, you can use the `DESTDIR` variable with make.

``` shell
make -C build DESTDIR=<prefix_path> install
```

Bazel: `bazelisk run --//:platform=gui //:install -- <prefix_path>`.
There is no Bazel equivalent of `DESTDIR`.
