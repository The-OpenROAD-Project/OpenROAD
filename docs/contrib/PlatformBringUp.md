# Guide to Integrate a New Platform into the OpenROAD Flow

## Table of Contents

- [Table of Contents](#table-of-contents)
- [Overview](#Overview)
- [Prerequisites](#Prerequisites)
- [Adding a new Platform to OpenROAD](#adding-a-new-platform-to-openroad)
  - [Setup](#Setup)
    - [Makefile](#Makefile)
    - [Platform Directory](#platform-directory)
    - [Design Directory](#design-directory)
  - [Platform Configuration](#platform-configuration)
  - [Design Configuration](#design-configuration)
    - [`config.mk`](#config-mk)
    - [`constraint.sdc`](#constraint-sdc)
    - [Liberty, LEF, and GDS Files](#liberty-lef-and-gds-files)
  - [Behavioral Models](#behavioral-models)
    - [Gated Clock](#gated-clock)
    - [Latch](#latch)
  - [FastRoute Configuration](#fastroute-configuration)
  - [Metal Tracks Configuration](#metal-tracks-configuration)
  - [PDN Configuration](#pdn-configuration)
  - [Tapcell Configuration](#tapcell-configuration)
  - [`setRC` Configuration](#setrc-configuration)
  - [KLayout](#klayout)
    - [KLayout properties file](#klayout-properties-file)
    - [KLayout tech file](#klayout-tech-file)
- [Validating the New Platform](#validating-the-new-platform)
- [Authors/Contributors](#authors-contributors)

## Overview

This document is a guide for foundry and third party IP providers to
easily integrate and test a new technology in to the OpenROAD RTL to GDS
flow. OpenROAD allows you to integrate any PDK (Process Design Kit) for any
feature size and implement a fully open-sourced RTL-GDSII flow (synthesizable
Verilog to merged GDSII). The OpenROAD flow has been validated for feature
sizes down to 7nm and used to design and tapeout over 100 ASIC and SoCs
to date.

## Prerequisites

To build and add a new platform for OpenROAD, key technology and library
components must be provided based on the technology node. These are generally
available as part of the standard design kit provided by a foundry or a
third-party IP provider.

They include :

*   A standard cell library
    *   GDS files of all standard cells in the library (or a way to generate
        them from the layout files, e.g., Magic VLSI layout tool).
*   A technology LEF file of the PDK being used that includes all relevant
    information regarding metal layers, vias, and spacing requirements.
    *   See `flow/platforms/nangate45/lef/NangateOpenCellLibrary.tech.lef`
        as an example tech LEF file.
*   A macro LEF file of the standard cell kit that includes MACRO definitions
    of every cell, pin designations (input/output/inout).
    *   See `flow/platforms/nangate45/lef/NangateOpenCellLibrary.macro.lef`
        as an example macro LEF file.
*   A Liberty file of the standard cell library with PVT characterization,
    input and output characteristics, timing and power definitions for
    each cell.
    *   See `flow/platforms/nangate45/lib/NangateOpenCellLibrary_typical.lib`
        as an example liberty file.
*   For KLayout: A mapping from LEF/DEF to GDS `layers:datatypes`

Adding a new platform additionally requires the following:

*   A validated installation of the OpenROAD flow scripts is available. See
    instructions [here](../user/GettingStarted.md).
*   A general knowledge of VLSI design and RTL to GDS flows.  OpenROAD
    implements a fully-automated RTL-GDSII but it requires familiarity with
    the OpenROAD flow scripts to debug problems.

## Adding a New Platform to OpenROAD

### Setup

This section describes the necessary files and  directories needed to build
the platform.  All files and directories made/edited are independent of
each other unless otherwise stated.

### Makefile

Make the following edits  to the Makefile (located in `flow/Makefile`)
so that OpenROAD can run the flow on a design using the new platform.

At  the beginning of the Makefile, there is a block of `DESIGN_CONFIG`
variables that are commented out. These variables tell OpenROAD which
design to run and on what platform. `DESIGN_CONFIG` specifically points
to a `config.mk` file located in the designs directory for the respective
platform. It is not required to add a `DESIGN_CONFIG` variable for a design
in the respective platform directly into the Makefile. It is merely a
convenience to add a `DESIGN_CONFIG` variable in the `Makefile` and can
instead be set when invoking make. OpenROAD has multiple Verilog designs
already made which can be used with any platform (see `flow/designs/src`
for a list of usable designs). For example, a `DESIGN_CONFIG` variable
using the `gcd` design on a new platform would look as follows:

```
#Makefile
DESIGN_CONFIG=./designs/MyNewPlatform/gcd/config.mk
```

The `config.mk` file will be generated later in the [Design
Directory](#design-directory) section of this document.

### Platform Directory

Create a directory for the new technology inside `flow/platforms` to contain
the necessary files for the OpenROAD flow.

```
$ mkdir flow/platforms/MyNewPlatform
```

### Design Directory

The design directory contains the configuration files for all the designs of
a specific platform. Create a directory for the new platform in flow/designs
to contain the relevant files and directories for all the designs for the
flow in that specific platform. Each design requires its own `config.mk`
and `constraint.sdc` files.

Follow the steps below to create the necessary directories and files.
**NOTE: gcd is just an example and not a required name.**:

```
$ mkdir -p flow/designs/MyNewPlatform/gcd
$ touch flow/designs/MyNewPlatform/gcd/config.mk
$ touch flow/designs/MyNewPlatform/gcd/constraint.sdc
```

This creates two directories MyNewPlatform and `gcd` and two empty files
`config.mk` and `constraint.sdc` in `flow/designs/MyNewPlatform/gcd`.

### Platform Configuration

This section describes the necessary files in the platform directory needed
for the OpenROAD flow. Specifically the `config.mk` file in the platform
directory has all of the configuration variables that the flow uses. Refer
to the OpenROAD-flow-scripts documentation for a full list of configuration
variables that can be set.
Refer to the [Flow variables](../user/FlowVariables.md) document for details on how to use 
environment variables in OpenROAD-flow-scripts to configure platform and design specific parameters. 

For an example of a platform `config.mk` file, refer to
`flow/platforms/sky130hd/config.mk.`

### Design Configuration

This section describes files in the design directory.

### `config.mk`

The `config.mk` file describes design-specific variables.

For Example:

```
    DESIGN_NAME
    PLATFORM
    VERILOG_FILES
    SDC_FILE
    CORE_UTILIZATION
    CORE_ASPECT_RATIO
    CORE_MARGIN
    PLACE_DENSITY
```

Alternatively, `DIE_AREA` and `CORE_AREA` can be specified instead of
`CORE_UTILIZATION`, `CORE_ASPECT_RATIO`, and `CORE_MARGIN`. For a complete
descriptor of all variables see [here](TODO).

Following is a sample `config.mk` file for the `gcd` design:

```
#config.mk
###########################
export DESIGN_NAME     = gcd
export PLATFORM        = sky130hd

export VERILOG_FILES = $(sort $(wildcard ./designs/src/$(DESIGN_NAME)/*.v))

export SDC_FILE = ./designs/$(PLATFORM)/$(DESIGN_NAME)/constraint.sdc

export CORE_UTILIZATION  = 30
export CORE_ASPECT_RATIO = 1
export CORE_MARGIN       = 2
export PLACE_DENSITY     = 0.70
```

### `constraint.sdc`

The `constraint.sdc` file defines timing constraints for the design. The
`create_clock` command allows you to define clocks that are either connected
to nets or are virtual and can be customized. The units for `create_clock`
need to be consistent with the liberty time units. Here’s an example of
a `constraint.sdc` file which defines a clock `clk` with a period of 8.4
nanoseconds (nanoseconds being consistent with the liberty time units).

```
#constraint.sdc
############################
create_clock [get_ports clk] -period 8.4  #Units are in nanoseconds
```

Refer to the
[OpenSTA][https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/doc/OpenSTA.pdf]
for the full documentation of the `create_clock` command.

### Liberty, LEF, and GDS Files

The liberty, LEF, and GDS files do not technically have to reside inside the
platform directory of respective technology as long as the paths set in the
`config.mk` file point to the correct files. However, it is good practice to
have all relevant files in one localized directory. The `.lib`, `.lef`, and
`.gds` reside in directories named respectively for the specific technology.

For example:
```
$ mdkir flow/platforms/MyNewPlatform/lib
$ mdkir flow/platforms/MyNewPlatform/lef
$ mdkir flow/platforms/MyNewPlatform/gds
```

A merged GDS file may be used instead of adding every individual `.gds`
file from the standard cell library.

Once the liberty file, tech and macro LEF files, and either the merged
standard cell GDS or individual standard cell GDS files have been generated,
place them in their respective directories and set the `lib`, `lef`, and
`gds` variables in the platform `config.mk` file to the correct paths.

### Clock Gates

--------------------------------------------------------------------------------
Yosys cannot (currently) infer clock gates automatically. However, users can
manually instantiate clock gates in their RTL using a generic interface. The
purpose of this interface is to separate platform-specific RTL (also called
"hardened" RTL) from platform-independent RTL (generic RTL).

This file is only required if you want to instantiate clock gates in your
design.

To create this module, a gated clock standard cell is required. This standard
cell is used to create the generic module `OPENROAD_CLKGATE`, as shown below.

```
// cells_clkgate.v
//////////////////////////
module OPENROAD_CLKGATE (CK, E, GCK);
  input  CK;
  input  E;
  output GCK;

  <clkgate_std_cell> latch (.CLK(CK), .GATE(E), .GCLK(GCK));
endmodule
```

An example instantiation of this module in a user design is shown below.

```
// buffer.v
// This is not a platform file, this is an example user design
//////////////////////////

module buffer (clk, enable, in, out);

input        clk, enable;
input  [7:0] in,
output [7:0] out

reg  [15:0] buffer_reg;
wire        gck; // Gated clock

OPENROAD_CLKGATE clkgate (.CK(clk), .E(enable), .GCK(gck));

// Buffer does not change if enable is low
always @(posedge gck) begin
  buffer_reg[15:8] <= in;
  buffer_reg[ 7:0] <= buffer_reg[15:8];
end

assign out = buffer_reg[ 7:0];

```

### Latches

Yosys can automatically infer latches from RTL, however it requires a behavioral
Verilog module. Example latch definitions are provided below. `DLATCH_P` is an
active-high level-sensitive latch and `DLATCH_N` is an active-low
level-sensitive latch.

This file is only required if you want to infer latches for your design.

```
// cells_latch.v
//////////////////////////
module $_DLATCH_P_(input E, input D, output Q);
  <d_latch_std_cell> _TECHMAP_REPLACE_ (
    .D (D),
    .G (E),
    .Q (Q)
  );
endmodule

module $_DLATCH_N_(input E, input D, output Q);
  <d_latch_std_cell> _TECHMAP_REPLACE_ (
    .D  (D),
    .GN (E),
    .Q  (Q)
  );
endmodule
```

### FastRoute Configuration

FastRoute is the tool used to global-route the design. FastRoute requires a
Tcl file to set which routing layers will be used for signals, adjust routing
layer resources, set which routing heuristic to use when routing, etc. It’s
recommended to use the default `fastroute.tcl` due to its simplicity and
effectiveness. Following is the default FastRoute configuration file.

```
# fastroute.tcl
#####################
set_global_routing_layer_adjustment $::env(MIN_ROUTING_LAYER)-$::env(MAX_ROUTING_LAYER) 0.5

set_routing_layers -signal $::env(MIN_ROUTING_LAYER)-$::env(MAX_ROUTING_LAYER)
```

The first command, `set_global_routing_layer_adjustment`, adjusts the
routing resources of the design. This effectively reduces the number of
routing tracks that the global router assumes to exist. By setting it to
the value of 0.5, this reduced the routing resources of all routing layers
to 50% which can help with congestion and reduce the challenges for detail
routing. The second command, `set_routing_layers`, sets the minimum and
maximum routing layers for signal nets by using the `-signal` option.

More customization can be done to increase the efficiency of global and
detail route. Refer to the [FastRoute documentation](../main/src/grt/README.md)

### Metal Tracks Configuration

OpenROAD requires a metal track configuration file for use in
floorplanning. For each metal layer, the x and y offset as well as the x and
y pitch are defined. To find the pitch and offset for both x and y, refer
to the `LAYER` definition section for each metal in the tech LEF. Following
is a generalized metal tracks configuration file with five metal tracks
defined. **Units are in microns**.

```
# make_tracks.tcl
###############################
make_tracks metal1 -x_offset 0.24 -x_pitch 0.82 -y_offset 0.24 -y_pitch 0.82
make_tracks metal2 -x_offset 0.28 -x_pitch 0.82 -y_offset 0.28 -y_pitch 0.82
make_tracks metal3 -x_offset 0.28 -x_pitch 0.82 -y_offset 0.28 -y_pitch 0.82
make_tracks metal4 -x_offset 0.28 -x_pitch 0.82 -y_offset 0.28 -y_pitch 0.82
make_tracks metal5 -x_offset 0.28 -x_pitch 0.82 -y_offset 0.28 -y_pitch 0.82
```

Following is the `LAYER` definition for `metal1` in the `sky130hd` tech LEF.

```
LAYER met1
  TYPE ROUTING ;
  DIRECTION HORIZONTAL ;

  PITCH 0.34 ;
  OFFSET 0.17 ;

  WIDTH 0.14 ;                     # Met1 1
  # SPACING 0.14 ;                 # Met1 2
  # SPACING 0.28 RANGE 3.001 100 ; # Met1 3b
  SPACINGTABLE
     PARALLELRUNLENGTH 0
     WIDTH 0 0.14
     WIDTH 3 0.28 ;
  AREA 0.083 ;                     # Met1 6
  THICKNESS 0.35 ;
  MINENCLOSEDAREA 0.14 ;

  ANTENNAMODEL OXIDE1 ;
  ANTENNADIFFSIDEAREARATIO PWL ( ( 0 400 ) ( 0.0125 400 ) ( 0.0225 2609 ) ( 22.5 11600 ) ) ;

  EDGECAPACITANCE 40.567E-6 ;
  CAPACITANCE CPERSQDIST 25.7784E-6 ;
  DCCURRENTDENSITY AVERAGE 2.8 ; # mA/um Iavg_max at Tj = 90oC
  ACCURRENTDENSITY RMS 6.1 ; # mA/um Irms_max at Tj = 90oC
  MAXIMUMDENSITY 70 ;
  DENSITYCHECKWINDOW 700 700 ;
  DENSITYCHECKSTEP 70 ;

  RESISTANCE RPERSQ 0.125 ;
END met1
```

In the example above, the x and y pitch for `met1` would be 0.34 and the
x and y offset would be 0.17.

### PDN Configuration

PDN is a utility that simplifies adding a power grid into the floorplan. With
specifications given in the PDN configuration file, like which layer to use,
stripe width and spacing, the utility can generate the metal straps used
for the power grid. To create and configure a power grid, refer to the
[PDN documentation](../main/src/pdn/doc/PDN.md).

### Tapcell Configuration

The tapcell configuration file is used to insert tapcells and endcaps into
the design. Refer to the [Tapcell](../main/src/tap/README.md) documentation
on how to construct this file.

### setRC Configuration

`setRC` allows the user to define resistances and capacitances for layers
and vias using the `set_layer_rc` command. There is also a command that
allows you to set the resistance and capacitance of routing wires using
the `set_wire_rc`. The units `set_wire_rc` is expecting are per-unit-length
values. Often, per-unit-length values are available in the PDK user guide. For
`set_layer_rc`, Liberty units need to be used. Following is a generic example
of a `setRC` configuration file which sets the resistance and capacitance
of five metal layers, four vias, one signal wire, and one clock wire.

```
# setRC.tcl
#######################
set_layer_rc -layer M1 -capacitance 1.449e-04 -resistance 8.929e-04
set_layer_rc -layer M2 -capacitance 1.331e-04 -resistance 8.929e-04
set_layer_rc -layer M3 -capacitance 1.464e-04 -resistance 1.567e-04
set_layer_rc -layer M4 -capacitance 1.297e-04 -resistance 1.567e-04
set_layer_rc -layer M5 -capacitance 1.501e-04 -resistance 1.781e-05

set_layer_rc -via V1 -resistance 9.249146E-3
set_layer_rc -via V2 -resistance 4.5E-3
set_layer_rc -via V3 -resistance 3.368786E-3
set_layer_rc -via V4 -resistance 0.376635E-3

set_wire_rc -signal -layer M2
set_wire_rc -clock -layer M5
```

### KLayout

KLayout is used in the OpenROAD flow to provide GDS merging, DRC, and
LVS. Two files are required for KLayout and they are generated within the
KLayout GUI. Install KLayout on the host machine since it is not included
in the OpenROAD build process. Then create the properties and tech files
as explained below.

### KLayout tech file

Follow these steps to generate the KLayout tech file:

1. Open KLayout in a terminal.
2. Go to Tools -> Manage Technologies.
3. Click the + in the bottom left corner to create a new technology.
4. Set the name for the technology in the box that pops up. You should now see the technology name in the list on the left hand side.
5. Expand the technology by hitting the arrow and click on General.
6. Set the base path your platform directory and load the `.lyp` layer properties file that was generated earlier.
7. On the left hand side under your new technology click Reader Options and then click LEF/DEF on the top bar.
8. In the LEF+Macro Files section, you add the LEF files by clicking the + button on the right hand side of the box.
    1. **Note**: Only add your original merged LEF file. Make sure it includes the full path to the LEF file.
9. In the Production section, scroll down and add the layer map file by hitting the Load File button.
    1. **Note**: Make sure it includes the full path.
10. Above in the same section, change the layer name suffix and GDS data type to correspond with the layer map.
11. Generate the `.lyt` file by right clicking on the new technology name and click on Export Technology.
12. Save with the extension `.lyt`.

### KLayout properties file

The properties file is not required to obtain a GDS and is merely used for styling purposes inside. Follow these steps to generate the KLayout properties file:

1. Open KLayout.
2. Install the `tf_import` package.
    1. Inside KLayout, go to Tools.
    2. Manage Packages.
    3. Install New Packages.
    4. Select `tf_import`.
        1. If the source of the package is from GitHub, then the file “” needs to be edited to include “source stdio”.
3. Re-start KLayout.
4. File -> Import some LEF. Does not matter what LEF; you will just get an error message without one..
  1. Once selected, go to Options at the bottom left.
  2. Select your layer map file under the Production tab.
  3. Go to the LEF+Macro Files tab, then add under Additional LEF files, the merged (original) LEF file in your platform directory.
  4. Under Macro Layout Files, add the GDS file in your platform directory.
5. File -> Import Cadence tech file.
    1. You have to select a tech file (found in the PDK, usually inside the Virtuoso folder).
    2. KLayout also needs a `.drf` file which is automatically included if it resides in the same directory the cadence tech file was found in (found in the PDK’s Virtuoso folder)..
6. File -> Save Layer Properties.
  1. Save as a `.lyp` file in your platform directory.

## Validating the New Platform

To validate the new platform, simply run a design through the flow using
the new platform. The Makefile should already include the `DESIGN_CONFIG`
variables for the new platform which were generated in the Setup section
of the document. Simply uncomment a `DESIGN_CONFIG` variable for the new
platform in the Makefile, save, and then run `make` in the terminal to run
the design through the flow. Try a small design first (i.e. `gcd`) so that
run time is small and you can identify and fix errors faster.

## Authors/Contributors

*   James Stine - Oklahoma State University
*   Teo Ene - Oklahoma State University
*   Ricardo Hernandez - Oklahoma State University
*   Ryan Ridley - Oklahoma State University
*   Indira Iyer - OpenROAD Project Consultant
