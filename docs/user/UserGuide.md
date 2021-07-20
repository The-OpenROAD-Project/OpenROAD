# User Guide

The OpenROAD Project uses three tools to achieve RTL-to-GDS:

1.  [yosys](https://github.com/The-OpenROAD-Project/yosys): Logic
    Synthesis
2.  [OpenROAD App](https://github.com/The-OpenROAD-Project/OpenROAD):
    Floorplanning through Detailed Routing
3.  [KLayout](https://www.klayout.de/): GDS merge, DRC and LVS (public
    PDKs)

To automate RTL-to-GDS we provide [OpenROAD
Flow](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts),
which contains scripts that integrate the three tools.

## Code Organization

The [OpenROAD
Flow](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts)
repository serves as an example RTL-to-GDS flow using the OpenROAD
tools. The script `build_openroad.sh` in the repository will
automatically build the OpenROAD toolchain.

The two main directories are:

1. `tools/`: contains the source code for the entire yosys and
   [OpenROAD App](https://github.com/The-OpenROAD-Project/OpenROAD)
   (both via submodules) as well as other tools required for the flow.
3. `flow/`: contains reference recipes and scripts to run designs
   through the flow. It also contains public platforms and test
   designs.

## Setup

See [Getting Started](GettingStarted.md) guide.

## Using the OpenROAD Flow

See the [flow README](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/blob/master/flow/README.md)
for details about the flow and how to run designs through the flow.

## Using the OpenROAD App

See the [app README](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/README.md)
for details about the app and the available features and commands.
