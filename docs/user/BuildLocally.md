# Build from sources locally

## Prerequisites

### OpenROAD

Dependencies for OpenROAD app are documented in the script below.
During initial setup or if you have a new machine, run this script:

``` shell
# either run as root or use sudo
./etc/DependencyInstaller.sh
```

### KLayout

OpenROAD Flow requires [KLayout](https://www.klayout.de) `v0.27.1`.

### Flow Scripts

- `libXScrnSaver`
- `libXft`
- `libffi-devel`
- `python3`
- `python3-pip`
    - Use pip to install pandas: `pip3 install --user pandas`
- `qt5-qtbase`
- `tcl`
- `time`
- `which`

## Clone and Build

``` shell
$ git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts
$ cd OpenROAD-flow-scripts
$ ./build_openroad.sh --local
```

## Verify Installation

The binaries should be available on your `$PATH` after setting up the
environment.

``` shell
$ source ./setup_env.sh
$ yosys -help
$ openroad -help
$ exit
```
