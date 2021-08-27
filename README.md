# OpenROAD

[![Build Status](https://jenkins.openroad.tools/buildStatus/icon?job=OpenROAD-Public%2Fmaster)](https://jenkins.openroad.tools/job/OpenROAD-Public/job/master/) [![Coverity Scan Status](https://scan.coverity.com/projects/the-openroad-project-openroad/badge.svg)](https://scan.coverity.com/projects/the-openroad-project-openroad) [![Documentation Status](https://readthedocs.org/projects/openroad/badge/?version=latest)](https://openroad.readthedocs.io/en/latest/?badge=latest)

The documentation is also available [here](https://openroad.readthedocs.io/en/latest/).

## Install dependencies

For a limited number of configurations the following script can be used
to install dependencies. The script `etc/DependencyInstaller.sh` supports
Centos7 and Ubuntu 20.04. You need root access to correctly install the
dependencies with the script.

``` shell
./etc/DependencyInstaller.sh -help

Usage: ./etc/DependencyInstaller.sh -run[time]      # installs dependencies to run a pre-compiled binary
       ./etc/DependencyInstaller.sh -dev[elopment]  # installs dependencies to compile the openroad binary

```

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

| Argument             | Value                     |
|----------------------|---------------------------|
| CMAKE_BUILD_TYPE     | DEBUG, RELEASE            |
| CMAKE_CXX_FLAGS      | Additional compiler flags |
| TCL_LIB              | Path to tcl library       |
| TCL_HEADER           | Path to tcl.h             |
| ZLIB_ROOT            | Path to zlib              |
| CMAKE_INSTALL_PREFIX | Path to install binary    |

### Build by hand

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
./etc/Build.sh --cmake="-DCMAKE_BUILD_TYPE=DEBUG -DTCL_LIB=/path/to/tcl/lib"
```

The default install directory is `/usr/local`.
To install in a different directory use:

``` shell
./etc/Build.sh --cmake="-DCMAKE_INSTALL_PREFIX=<prefix_path>"
```

## Regression Tests

There are a set of regression tests in `test/`.

``` shell
# run all tool unit tests
test/regression
# run all flow tests
test/regression flow
# run <tool> tests
test/regression <tool>
# run <tool> tool tests
src/<tool>/test/regression
```

The flow tests check results such as worst slack against reference values.
Use `report_flow_metrics [test]...` to see the all of the metrics.
Use `save_flow_metrics [test]...` to add margins to the metrics and save them to <test>.metrics_limits.

``` text
% report_flow_metrics gcd_nangate45
                       insts    area util slack_min slack_max  tns_max clk_skew max_slew max_cap max_fanout DPL ANT drv
gcd_nangate45            368     564  8.8     0.112    -0.015     -0.1    0.004        0       0          0   0   0   0
```

## Run

``` text
openroad
  -help              show help and exit
  -version           show version and exit
  -no_init           do not read ~/.openroad init file
  -no_splash         do not show the license splash at startup
  -threads count     max number of threads to use
  -exit              exit after reading cmd_file
  cmd_file           source cmd_file
```

OpenROAD sources the Tcl command file `~/.openroad` unless the command
line option `-no_init` is specified.

OpenROAD then sources the command file cmd_file if it is specified on
the command line. Unless the `-exit` command line flag is specified it
enters an interactive Tcl command interpreter.

#### Run in Python

OpenROAD can run with a python command interpreter as following:
```
openroad -python [args]
```

Currently, OpenDB is the only module supported in python. You can see examples of python scripts in [OpenDB's python unit-tests](src/odb/test/unitTestsPython/).


Below is a list of the available tools/modules included in the OpenROAD app.

### OpenROAD (global commands)

- [OpenROAD](./src/README.md)

### Database

- [OpenDB](./src/odb/README.md)

### Parasitics Extraction

- [OpenRCX](./src/rcx/README.md)

### Synthesis

- [Restructure](./src/rmp/README.md)

### Initialize Floorplan

- [Floorplan](./src/ifp/README.md)

### Pin placement

- [ioPlacer](./src/ppl/README.md)

### Chip level connections

- [ICeWall](./src/ICeWall/README.md)

### Macro Placement

- [TritonMacroPlacer](./src/mpl/README.md)

### Tapcell

- [Tapcell](./src/tap/README.md)

### PDN analysis

- [PDN](./src/pdn/README.md)
- [PDNSim](./src/psm/README.md)

### Global Placement

- [RePlAce](./src/gpl/README.md)

### Timing Analysis

- [OpenSTA](src/sta/README.md)

### Gate Resizer

- [Resizer](./src/rsz/README.md)

### Detailed Placement

- [OpenDP](./src/dpl/README.md)

### Clock Tree Synthesis

- [TritonCTS 2.0](./src/cts/README.md)

### Global Routing

- [FastRoute](./src/grt/README.md)
- [Antenna Checker](./src/ant/README.md)

### Detailed Router

- [TritonRoute](./src/drt/README.md)

### Metal Fill

- [Metal Fill](./src/fin/README.md)

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
