# Build from sources locally

## Prerequisites

### OpenROAD

Dependencies for OpenROAD app are documented in the script below.
During initial setup or if you have a new machine, run this script:

``` shell
# either run as root or use sudo
./etc/DependencyInstaller.sh -dev
```

### KLayout

OpenROAD Flow requires [KLayout](https://www.klayout.de) `v0.27.1`.

### Packages

-   `libffi-devel`
-   `tcl`
-   `time`
-   `pandas` Python library
      -   Available from pip (e.g., `python3-pip`) to install pandas:
          `pip3 install --user pandas`

## Clone and Build

``` shell
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts
cd OpenROAD-flow-scripts
./build_openroad.sh --local
```

## Verify Installation

The binaries should be available on your `$PATH` after setting up the
environment.

``` shell
source ./setup_env.sh
yosys -help
openroad -help
exit
```
