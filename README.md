# OpenROAD

[![Build Status](https://jenkins.openroad.tools/buildStatus/icon?job=OpenROAD-Public%2Fmaster)](https://jenkins.openroad.tools/job/OpenROAD-Public/job/master/) [![Coverity Scan Status](https://scan.coverity.com/projects/the-openroad-project-openroad/badge.svg)](https://scan.coverity.com/projects/the-openroad-project-openroad) [![Documentation Status](https://readthedocs.org/projects/openroad/badge/?version=latest)](https://openroad.readthedocs.io/en/latest/?badge=latest) [![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5370/badge)](https://bestpractices.coreinfrastructure.org/projects/5370)

# Introduction

[OpenROAD](https://theopenroadproject.org/) is the leading
open-source, foundational application for semiconductor digital design.
It eliminates the barriers of cost, risk and uncertainty in hardware
design to foster open access, expertise, rapid innovation and faster
design turnaround. The OpenROAD application enables flexible flow
control through an API, based on Tcl and Python.

OpenROAD is a key tool for popular open source digital flows like
OpenROAD-flow-scripts,
[OpenLane](https://github.com/The-OpenROAD-Project/OpenLane) from
[Efabless](https://efabless.com/), Silicon Compiler Systems; as
well as [OpenFASoC](https://github.com/idea-fasoc/OpenFASOC) for
mixed-signal design flows.

OpenROAD users range from hardware designers, industry collaborators,
enthusiasts, academia and researchers.

Two main flow controllers are supported by the
[OpenROAD](https://github.com/The-OpenROAD-Project/OpenROAD)
project repository:

-   [OpenROAD-flow-scripts](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts) -
     Supported by the OpenROAD project

-   [OpenLane](https://github.com/The-OpenROAD-Project/OpenLane) -
     Supported by [Efabless](https://efabless.com/)

The OpenROAD flow delivers an autonomous, no-human-in-the-loop, 24 hr
turnaround from RTL-GDSII for design exploration and physical design
implementation.

![rtl2gds.webp](./docs/images/rtl2gds.webp)

## GUI

The OpenROAD GUI is a powerful visualization, analysis and debugging
tool with a customizable Tcl interface.Figures show GUI views for various
flow stages including post-routed timing, placement congestion, CTS.

![ibexGui.webp](./docs/images/ibexGui.webp)

## Placement Congestion View:

![pl_congestion.webp](./docs/images/pl_congestion.webp)

## CTS:

![clk_routing.webp](./docs/images/clk_routing.webp)

# PDK Support

The OpenROAD application is PDK independent. However, it has been tested
and validated with specific PDKs in the context of various flow
controllers.

OpenLane supports Skywater130.

OpenROAD-flow-scripts supports several public and private PDKs
including:

## Open Source PDKs

-   `Skywater130` - 130nm
-   `Nangate45` - 45nm
-   `ASAP7` - Predictive FinFET 7nm

## Proprietary PDKs

These PDKS are supported in OpenROAD-flow-scripts only. They are used to
test and calibrate OpenROAD against commercial platforms and ensure good
QoR. The PDKs and platform-specific files for these kits cannot be
provided due to NDA restrictions. However, if you are able to access
these platforms independently, you can create the necessary
platform-specific files yourself.

-   `GF55` - 55nm
-   `GF12` - 12nm
-   `GF180` - 180nm
-   `Intel22` - 22nm
-   `Intel16` - 16nm
-   `TSMC65` - 65nm

# Tapeouts

OpenROAD was used for full physical implementation in over 240 tapeouts
in Sky130 through the Google sponsored, Efabless [MPW
shuttle](https://efabless.com/open_shuttle_program) and
[ChipIgnite](https://efabless.com/) programs.

![shuttle.webp](./docs/images/shuttle.webp)

## OpenTitan SoC on GF12LP - Physical design and optimization using OpenROAD

![gf12lp.webp](./docs/images/gf12lp.webp)

## Continuous Tapeout Integration into CI

The OpenROAD project actively adds successfully taped out MPW shuttle
designs to the [CI regression
testing](https://github.com/The-OpenROAD-Project/OpenLane-MPW-CI).
Examples of designs include Open processor cores, RISC-V based SoCs,
cryptocurrency miners, robotic app processors, amateur satellite radio
transceivers, OpenPower based Microwatt etc.

The documentation is also available [here](https://openroad.readthedocs.io/en/latest/main/README.html).

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

| Argument               | Value                     |
|------------------------|---------------------------|
| `CMAKE_BUILD_TYPE`     | DEBUG, RELEASE            |
| `CMAKE_CXX_FLAGS`      | Additional compiler flags |
| `TCL_LIBRARY`          | Path to Tcl library       |
| `TCL_HEADER`           | Path to `tcl.h`           |
| `ZLIB_ROOT`            | Path to `zlib`            |
| `CMAKE_INSTALL_PREFIX` | Path to install binary    |

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
./etc/Build.sh -cmake="-DCMAKE_BUILD_TYPE=DEBUG -DTCL_LIB=/path/to/tcl/lib"
```

The default install directory is `/usr/local`.
To install in a different directory use:

``` shell
./etc/Build.sh -cmake="-DCMAKE_INSTALL_PREFIX=<prefix_path>"
```

### LTO Options
By default, OpenROAD is built with link time optimizations enabled. This adds 
about 1 minute to compile times and improves the runtime by about 11%. If
you would like to disable LTO pass `-DLINK_TIME_OPTIMIZATION=OFF` when
generating a build.

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
openroad [-help] [-version] [-no_init] [-exit] [-gui]
         [-threads count|max] [-log file_name] cmd_file
  -help              show help and exit
  -version           show version and exit
  -no_init           do not read .openroad init file
  -threads count|max use count threads
  -no_splash         do not show the license splash at startup
  -exit              exit after reading cmd_file
  -gui               start in gui mode
  -python            start with python interpreter [limited to db operations]
  -log <file_name>   write a log in <file_name>
  cmd_file           source cmd_file
```

OpenROAD sources the Tcl command file `~/.openroad` unless the command
line option `-no_init` is specified.

OpenROAD then sources the command file `cmd_file` if it is specified on
the command line. Unless the `-exit` command line flag is specified it
enters and interactive Tcl command interpreter.

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

- [ICeWall](./src/pad/README.md)

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

### Graphical User Interface

- [GUI](./src/gui/README.md)

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
