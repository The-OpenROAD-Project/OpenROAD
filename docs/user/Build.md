# Build OpenROAD

## Install dependencies

To install dependencies to develop or build and run OpenROAD locally,
follow [this procedure](https://openroad-flow-scripts.readthedocs.io/en/latest/user/BuildLocally.html).

> :warning: warning :warning:
>
> `etc/DependencyInstaller.sh` defaults to installing system 
> packages and requires sudo access. These packages can affect
> your environment. We recommend users install dependencies
> locally using [setup.sh](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/blob/master/setup.sh)
> from OpenROAD-flow-scripts.

## Build

The first step, independent of the build method, is to download the repository:

``` shell
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD.git
cd OpenROAD
```

OpenROAD git submodules (cloned by the `--recursive` flag) are located in `src/`.

The default build type is RELEASE to compile optimized code.
The resulting executable is in `build/src/openroad`.

Optional CMake variables passed as `-D<var>=<value>` arguments to CMake are show below.

| Argument               | Value                     |
|------------------------|---------------------------|
| `CMAKE_BUILD_TYPE`     | DEBUG, RELEASE            |
| `CMAKE_CXX_FLAGS`      | Additional compiler flags |
| `TCL_LIBRARY`          | Path to Tcl library       |
| `TCL_HEADER`           | Path to `tcl.h`           |
| `ZLIB_ROOT`            | Path to `zlib`            |
| `CMAKE_INSTALL_PREFIX` | Path to install binary    |

> **Note:** There is a `openroad_build.log` file that is generated
with every build in the build directory. In case of filing issues,
it can be uploaded in the "Relevant log output" section of OpenROAD
[issue forms](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/issues/new/choose).

### Build Manually

``` shell
mkdir build
cd build
cmake ..
make
```

The default install directory is `/usr/local`.
To install in a different directory with CMake use:

``` shell
cmake .. -DCMAKE_INSTALL_PREFIX=<prefix_path>
```

Alternatively, you can use the `DESTDIR` variable with make.

``` shell
make DESTDIR=<prefix_path> install
```

### Build using support script

``` shell
./etc/Build.sh
# To build with debug option enabled and if the Tcl library is not on the default path
./etc/Build.sh -cmake="-DCMAKE_BUILD_TYPE=DEBUG -DTCL_LIB=/path/to/tcl/lib"
```

The default install directory is `/usr/local`.
To install in a different directory use:

``` shell
./etc/Build.sh -cmake="-DCMAKE_INSTALL_PREFIX=<prefix_path>"
```

### LTO Options
By default, OpenROAD is built with link time optimizations enabled.
This adds about 1 minute to compile times and improves the runtime
by about 11%. If you would like to disable LTO pass 
`-DLINK_TIME_OPTIMIZATION=OFF` when generating a build.
