# Installing OpenROAD

## Clone Repository

The first step, independent of the build method, is to download the repository:

``` shell
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD.git
cd OpenROAD
```

OpenROAD git submodules (cloned by the `--recursive` flag) are located in `src/`.

```{note}
There are three methods for building OpenROAD (in order of recommendation): prebuilt binaries, docker images, and finally, local build.  
```

## Build and install with Bazel

Build OpenROAD with GUI support and install into ../install/OpenROAD/bin

    bazelisk run --//:platform=gui //:install

## Build with Prebuilt Binaries

Courtesy of [Precision Innovations](https://precisioninno.com/), there are prebuilt binaries
of OpenROAD with self-contained dependencies released on a regular basis.
Refer to this [link](https://openroad-flow-scripts.readthedocs.io/en/latest/user/BuildWithPrebuilt.html) for instructions.

## Build with Docker

### Prerequisites

- For this method you only need to install
[Docker](https://docs.docker.com/engine/install) on your machine.
- Ensure that you have sufficient memory allocated to the Virtual Machine (VM)
as per our system [requirements](../index.md#system-requirements). Refer to
this [Docker guide](https://docs.docker.com/config/containers/resource_constraints/) for setting CPU cores and memory limits.

### Installation

We recommend to use a Docker image of a supported OS
and install OpenROAD using the prebuilt binaries from
Precision Innovations. 
You can start the container in an interactive mode using 
the command below. 

```shell
docker run -it ubuntu:22.04
```

Now you are ready to install the prebuilt binaries. 
Please refer to the instructions for installing prebuilt binaries 
[above](#build-with-prebuilt-binaries).

## Build Locally

The default build type is `RELEASE` to compile optimized code.
The resulting executable is in `build/bin/openroad`.

Optional CMake variables passed as `-D<var>=<value>` arguments to CMake are show below.

| Argument               | Value                     |
|------------------------|---------------------------|
| `CMAKE_BUILD_TYPE`     | DEBUG, RELEASE            |
| `CMAKE_CXX_FLAGS`      | Additional compiler flags |
| `TCL_LIBRARY`          | Path to Tcl library       |
| `TCL_HEADER`           | Path to `tcl.h`           |
| `ZLIB_ROOT`            | Path to `zlib`            |
| `CMAKE_INSTALL_PREFIX` | Path to install binary    |

```{note}
There is a `openroad_build.log` file that is generated
with every build in the build directory. In case of filing issues,
it can be uploaded in the "Relevant log output" section of OpenROAD
[issue forms](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/issues/new/choose).
```

### Install Dependencies

You may follow our helper script to install dependencies as follows:
``` shell
sudo ./etc/DependencyInstaller.sh -base
./etc/DependencyInstaller.sh -common -local
```

```{warning}
`sudo ./etc/DependencyInstaller.sh [-all|-common]` defaults to
installing packages on /usr/local.
To avoid this bahavior use -local flag or -prefix <PATH> argument.
```

### Build OpenROAD

To build with the default options in release mode:

``` shell
./etc/Build.sh
```

#### Custom Library Path

To build with debug option enabled and if the Tcl library is not on the default path.

``` shell
./etc/Build.sh -cmake="-DCMAKE_BUILD_TYPE=DEBUG -DTCL_LIB=/path/to/tcl/lib"
```

#### Enable `manpages`

To build the `manpages`:
``` shell
./etc/Build.sh -build-man
```

#### LTO Options

By default, OpenROAD is built with link time optimizations enabled.
This adds about 1 minute to compile times and improves the runtime
by about 11%. If you would like to disable LTO pass
`-DLINK_TIME_OPTIMIZATION=OFF` when generating a build.

#### Build with Address Sanitizer

To enable building with Address Sanitizer, use the argument `-DASAN=ON`.
Setting the `ASAN` variable to `ON` adds necessary compile and link options
for using Address Sanitizer.

```{note}
Address Sanitizer adds instrumentation for detecting memory errors.
Enabling this option will cause OpenROAD to run slower and consume more RAM.
```

#### System wide OpenROAD Install

```{warning}
Only use the following instructions if you know what you are doing.
```

``` shell
sudo make install
```

The default install directory is `/usr/local`.
To install in a different directory with CMake use:

``` shell
./etc/Build.sh -cmake="-DCMAKE_INSTALL_PREFIX=<prefix_path>"
```

Alternatively, you can use the `DESTDIR` variable with make.

``` shell
make -C build DESTDIR=<prefix_path> install
```
