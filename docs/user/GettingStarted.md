# Getting Started with OpenROAD Flow

OpenROAD Flow is a full RTL-to-GDS flow built entirely on open-source tools.
The project aims for automated, no-human-in-the-loop digital circuit design
with 24-hour turnaround time.

## Setup

### System Requirements

To build the binaries and run `gcd` through the flow:

- Minimum: 1 CPU core and 4GB RAM.
- Recommend: 4 CPU cores and 16GB of RAM.

---

**NOTE**

`gcd` is a small design, and thus requires less computational power.
Larger designs may require better hardware.

---

### Building and Installing the Software

There are currently two options to set up the OpenROAD Flow:

- Build from sources using Docker, [instructions here](./BuildWithDocker.md).
- Build from sources locally, [instructions here](./BuildLocally.md).


---

**WARNING**

On Centos7 you need to manually make sure the PATH variable includes the
new version of GCC/Clang. To enable GCC-8 or Clang-7 you need to run:

```shell
# enable gcc-8
source /opt/rh/devtoolset-8/enable
# enable clang-7
source /opt/rh/llvm-toolset-7.0/enable
```

---

## Running a Design

Sample design configurations are available in the `designs` directory.
You can select a design using either of the following methods:

1.  The flow
    [Makefile](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/blob/master/flow/Makefile)
    contains a list of sample design configurations at the top of the
    file. Uncomment the respective line to select the design.
2.  Specify the design using the shell environment. For example:

``` shell
make DESIGN_CONFIG=./designs/nangate45/swerv/config.mk
# or
export DESIGN_CONFIG=./designs/nangate45/swerv/config.mk
make
```

By default, the `gcd` design is selected using the
`nangate45` platform. The resulting GDS will be available at
`flow/results/nangate45/gcd/6_final.gds`. The flow should take only a few
minutes to produce a GDS for this design.  We recommend implementing this
design first to validate your flow and tool setup.

### Adding a New Design

To add a new design, we recommend looking at the included designs for
examples of how to set one up.

## Platforms

OpenROAD-flow-scripts supports Verilog to GDS for the following open platforms:

- ASAP7
- Nangate45 / FreePDK45
- SKY130

These platforms have a permissive license which allows us to
redistribute the PDK and OpenROAD platform-specific files. The platform
files and license(s) are located in `platforms/{platform}`.

OpenROAD-flow-scripts also supports the following commercial platforms:

- GF12
- TSMC65LP

The PDKs and platform-specific files for these kits cannot be provided
due to NDA restrictions. However, if you are able to access these
platforms, you can create the necessary platform-specific files
yourself.

Once the platform is set up, you can create a new design configuration with
information about the design. See sample configurations in the `design`
directory.

### Adding a New Platform

At this time, we recommend looking at the
[Nangate45](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/tree/master/flow/platforms/nangate45)
as an example of how to set up a new platform for OpenROAD-flow-scripts.

## Implement the Design

Run `make` to perform Verilog to GDS. The final output will be located
at `flow/results/{platform}/{design_name}/6_final.gds`

## Miscellaneous

### nangate45 smoke-test harness for top-level Verilog designs

1.  Drop your Verilog files into designs/src/harness
2.  Start the workflow:

---
**TIP!**

Start with a very small submodule in your design that has only a few pins.

---

``` shell
make DESIGN_NAME=TopLevelName DESIGN_CONFIG=`pwd`/designs/harness.mk
```
