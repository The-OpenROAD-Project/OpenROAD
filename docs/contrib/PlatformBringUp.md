# User Guide to Integrate a New Platform into the OpenROAD Flow

# Table of Contents <a name="TOC"></a>

[Table of Contents](#TOC)
[Overview](#Overview)
[Prerequisites](#Prerequisites)
[Adding a new Platform to OpenROAD](#APOpenROAD)
	[Setup](#Setup)
		[Makefile](#Makefile)
		[Platform Directory](#PlatformDirectory)
		[Design Directory](#DesignDirectory)
	[Platform Configuration](#PlatformConfiguration)
	[Design Configuration](#DesignConfiguration)
		[config.mk](#configmk)
		[constraint.sdc](#constraintsdc)
		[Liberty, LEF, and GDS Files](#LibertyLEFGDSFiles)
	[Behavioral Models](#BehavioralModels)
		[Gated Clock](#GatedClock)
		[Latch](#Latch)
	[FastRoute Configuration](#FastRouteConfiguration)
	[Metal Tracks Configuration](#MetalTracksConfiguration)
	[PDN Configuration](#PDNConfiguration)
	[Tapcell Configuration](#TapcellConfiguration)
	[setRC Configuration](#setRCConfiguration)
	[KLayout](#KLayout)
		[KLayout properties file](#KLayoutpropertiesfile)
		[KLayout tech file](#KLayouttechfile)
[Validating the New Platform](#ValidatingPlatform)
[Authors/Contributors](#AuthorsContributors)


# Overview <a name="Overview"></a>

This document is a guide for foundry and third party IP providers to easily integrate and test a new technology into the OpenROAD RTL to GDS flow. OpenROAD allows you to integrate any public PDK (Process Design Kit) for any feature size and implement a fully open-sourced RTL-GDSII flow (synthesizable Verilog to merged GDSII). The OpenROAD flow has been validated for feature sizes up to 7nm and used to design and tapeout over 100 ASIC and SoCs to date.


# Prerequisites <a name="Prerequisites"></a>

To build and add a new platform for OpenROAD, key technology and library components must be provided based on the technology node. These are generally available as part of the standard design kit provided by a foundry or a third-party IP provider.  

They include :

* A standard cell library
    * GDS files of all standard cells in the library (or a way to generate them from the layout files, eg. Magic VLSI layout tool)
* A technology LEF file of the PDK being used that includes all relevant information regarding metal layers, vias, and spacing requirements. 
    * Can be generated through the commercial tool Cadence Abstract.
    * See `OpenROAD-flow-scripts/flow/platforms/nangate45/lef/NangateOpenCellLibrary.tech.lef` as an example tech LEF file.
* A macro LEF file of the standard cell kit that includes MACRO definitions of every cell, pin designations (input/output/inout), and size data of metal layers with port connections.
    * Can be generated through the commercial tool Cadence Abstract.
    * See `OpenROAD-flow-scripts/flow/platforms/nangate45/lef/NangateOpenCellLibrary.macro.lef `as an example macro LEF file.
* A Liberty file of the standard cell library with PVT characterization, input and output characteristics, timing and power definitions for each cell.
    * See `OpenROAD-flow-scripts/flow/platforms/nangate45/lib/NangateOpenCellLibrary_typical.lib `as an example liberty file.

Adding a new platform additionally requires the following: 

* A validated installation of the OpenROAD flow scripts is available. See instructions [here](https://openroad.readthedocs.io/en/latest/user/GettingStarted.html#building-and-installing-the-software).:
* A general knowledge of VLSI design and RTL to GDS flows.  \
OpenROAD implements a fully-automated RTL-GDSII but it requires familiarity with the OpenROAD flow scripts flow and the scripts provided to enable debugging in case of problems.Add a flow section briefly describing the OR or ORFS flow steps to add the platform a platform here


# Adding a New Platform to OpenROAD <a name="ANPOpenROAD"></a>


## Setup <a name="Setup"></a>

This section describes the necessary files and  directories needed to build the platform.  All files and directories made/edited are independent of each other unless otherwise stated.


## Makefile <a name="Makefile"></a>

Make the following edits  to the Makefile (located in `OpenROAD-flow-scripts/flow/Makefile`) so that OpenROAD can run the flow on a design using the new platform.

1. At  the beginning of the Makefile, there is a block of `DESIGN_CONFIG` variables that are commented out. These variables tell OpenROAD which design to run and on what platform. `DESIGN_CONFIG` specifically points to a `config.mk` file located in the designs directory for the respective platform. A `DESIGN_CONFIG` variable must be added for the new platform using a specific design. OpenROAD has multiple verilog designs already made which can be used with any platform (see `OpenROAD-flow-scripts/flow/designs/src` for a list of usable designs). For example, a `DESIGN_CONFIG` variable using the `riscv32i` design on a new platform would look as follows:


```
DESIGN_CONFIG=./designs/MyNewPlatform/riscv32i/config.mk
```


The `config.mk` file will be generated later in the [Design Directory](#DesignDirectory) section of this document.


## Platform Directory <a name="PlatformDirectory"></a>

Create a directory for the new technology inside `OpenROAD-flow-scripts/flow/platforms` to contain the necessary files for the OpenROAD flow.


```
$ mkdir OpenROAD-flow-scripts/flow/platforms/MyNewPlatform
```



## Design Directory <a name="DesignDirectory"></a>

The design directory contains the configuration files for all the designs of a specific platform. Create a directory for the new platform in OpenROAD-flow-scripts/flow/designs to contain the relevant files and directories for all the designs for the flow in that specific platform. Each design requires it’s own config.mk and constraints.sdc files. 

Follow the steps below to create the necessary directories and files:


```
$ mkdir OpenROAD-flow-scripts/flow/designs/MyNewPlatform
$ mkdir OpenROAD-flow-scripts/flow/designs/MyNewPlatform/riscv32i
$ touch   OpenROAD-flow-scripts/flow/designs/MyNewPlatform/riscv32i/config.mk
$ touch OpenROAD-flow-scripts/flow/designs/MyNewPlatform/riscv32i/constraints.sdc
```


This creates two directories MyNewPlatform and riscv32i and two empty files config.mk and constraints.sdc in OpenROAD-flow-scripts/flow/designs/MyNewPlatform/riscv32i. 


## Platform Configuration <a name="PlatformConfiguration"></a>

This section describes the necessary files in the platform directory needed for the OpenROAD flow. Specifically the config.mk file in the platform directory has all of the configuration variables that the flow uses. Refer to the OpenROAD-flow-scripts documentation for a full list of configuration variables that can be setSpecifically the config.mk file in the platform directory has all of the configuration variables that the flow uses. Refer to the OpenROAD-flow-scripts documentation for a full list of configuration variables that can be set. 

For an example of a platform config.mk file, refer to `OpenROAD-flow-scripts/flow/platforms/sky130hd/config.mk.`


## Design Configuration <a name="DesignConfiguration"></a>

This section describes files in the design directory.


## config.mk <a name="configmk"></a>

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


Following is a sample `config.mk` file for the `riscv32i` design:


```
#config.mk
###########################
export DESIGN_NAME     = riscv
export PLATFORM        = sky130hd

export VERILOG_FILES = $(sort $(wildcard ./designs/src/$(DESIGN_NICKNAME)/*.v))

export SDC_FILE = ./designs/$(PLATFORM)/$(DESIGN_NICKNAME)/constraint.sdc

export CORE_UTILIZATION  = 30
export CORE_ASPECT_RATIO = 1
export CORE_MARGIN       = 2
export PLACE_DENSITY     = 0.70
```


Change these variables to suit the needs of the specific design being made.


## constraint.sdc <a name="constraintsdc"></a>

The `constraint.sdc` file defines timing constraints for the design. Here’s an example of a constraint.sdc file which defines a clock `clk `with a period of 8.4 nanoseconds.


```
#constraint.sdc
############################
create_clock [get_ports clk] -period 8.4
```


The `create_clock` command allows you to define clocks that are either connected to nets  or are virtual and can be customized.


```
#Create a netlist clock
create_clock -period <float> <netlist clock list of regexes>

#Create a virtual clock
create_clock -period <float> -name <virtual clock name>

#Create a netlist clock with custom waveform/duty-cycle
create_clock -period <float> -waveform {rising_edge falling_edge} <netlist clock list of regexes>
```



## Liberty, LEF, and GDS Files <a name="LibertyLEFGDSFiles"></a>

The liberty, LEF, and GDS files do not technically have to reside inside the platform directory of respective technology as long as the paths set in the `config.mk` file point to the correct files. However, it is good practice to have all relevant files in one localized directory. The` .lib, .lef, .gds `reside in directories named respectively for the specific technology.

For example:


```
$ mdkir OpenROAD-flow-scripts/flow/platforms/MyNewPlatform/lib
$ mdkir OpenROAD-flow-scripts/flow/platforms/MyNewPlatform/lef
$ mdkir OpenROAD-flow-scripts/flow/platforms/MyNewPlatform/gds
```


A merged gds file may be used instead of adding every individual .gds file from the standard cell library. This can be done using calibredrv using the following Tcl script:


```
# merge_gds.tcl
######################
layout filemerge -in <gds file of Liberty, LEF, and GDS Filescell 1> \
 -in <gds file of cell 2> \
 -in <gds file of cell 3> \
 .
 .
 .
 -in <gds file of cell n> \ 
 -out merged.gds
```


Run this in a tcl command shell to create a `merged.gds`:


```
$ calibredrv merged_gds.tcl
```


	

Once the liberty file, tech and macro lef files, and either the merged standard cell gds or individual standard cell gds files have been generated, place them in their respective directories and set the lib, lef, and gds variables in the platform `config.mk` file to the correct paths.


## Behavioral Models <a name="BehavioralModels"></a>

OpenROAD requires behavioral verilog modules for a latch and a gated clock.These modules are used during synthesis and follow a specific naming convention.


## Gated Clock <a name="GatedClock"></a>

To create this module, a gated clock standard cell is required. This standard cell is used to create the OpenROAD specific module. Following is the generic structure of the behavioral latch module:


```
# cells_clkgate.v
##########################
module OPENROAD_CLKGATE (CK, E, GCK);
  input CK;
  input  E;
  output GCK;

 `ifdef OPENROAD_CLKGATE
    <clkgate_std_cell> latch (.CLK(CK), .GATE(E), .GCLK(GCK));
 `else
    assign GCK = CK;
 `endif
endmodule
```



## Latch <a name="Latch"></a>

Next is the generic latch. Once again, this requires a latch standard cell to be used. The structure of this module differs slightly from the clkgate module seen previously. Following is the generic structure of the behavioral latch module:

		


```
# cells_latch.v
########################
module $_DLATCH_P_(input E, input D, output Q);
     <d_latch_std_cell> _TECHMAP_REPLACE_ (
       .D(D),
       .G(E),
       .Q(Q)
       );
endmodule

module $_DLATCH_N_(input E, input D, output Q);
     <d_latch_std_cell> _TECHMAP_REPLACE_ (
       .D(D),
       .GN(E),
       .Q(Q)
       );
endmodule
```


This file contains two modules, $_DLATCH_P_ and $_DLATCH_N_. Notice that $_DLATCH_N_ has it’s gate input is negated making it enable on a low signal while $_DLATCH_P_ has a non-negated gate input making it enable on a high signal.


## FastRoute Configuration <a name="FastRouteConfiguration"></a>

FastRoute is the tool used to global-route the design. FastRoute requires a Tcl file to set which routing layers will be used for signals, adjust routing layer resources, set which routing heuristic to use when routing, etc. It’s recommended to use the default fastroute.tcl due to its simplicity and effectiveness. Following is the default FastRoute configuration file.


```
# fastroute.tcl
#####################
set_global_routing_layer_adjustment $::env(MIN_ROUTING_LAYER)-$::env(MAX_ROUTING_LAYER) 0.5

set_routing_layers -signal $::env(MIN_ROUTING_LAYER)-$::env(MAX_ROUTING_LAYER)
```


The first command, set_global_routing_layer_adjustment, adjusts the routing resources of the design. This effectively reduces the number of routing tracks that the global router assumes to exist. By setting it to the value of 0.5, this reduced the routing resources of all routing layers to 50% which can help with congestion and reduce the challenges for detail routing. The second command, set_routing_layers, sets the minimum and maximum routing layers for signal nets by using the -signal option.

More customization can be done to increase the efficiency of global and detail route. Refer to the [OpenROAD documentation](https://openroad.readthedocs.io/_/downloads/en/latest/pdf/).


## Metal Tracks Configuration <a name="MetalTracksConfiguration"></a>

OpenROAD requires a metal track configuration file for use in floorplanning. For each metal layer, the x and y offset as well as the x and y pitch are defined. To find the pitch and offset for both x and y, refer to the tech LEF. Following is a generalized metal tracks configuration file with five metal tracks defined.


```
# make_tracks.tcl
###############################
make_tracks metal1 -x_offset 0.24 -x_pitch 0.82 -y_offset 0.24 -y_pitch 0.82
make_tracks metal2 -x_offset 0.28 -x_pitch 0.82 -y_offset 0.28 -y_pitch 0.82
make_tracks metal3 -x_offset 0.28 -x_pitch 0.82 -y_offset 0.28 -y_pitch 0.82
make_tracks metal4 -x_offset 0.28 -x_pitch 0.82 -y_offset 0.28 -y_pitch 0.82
make_tracks metal5 -x_offset 0.28 -x_pitch 0.82 -y_offset 0.28 -y_pitch 0.82
```


Refer to the tech file to determine the x and y pitch and the offset.


## PDN Configuration <a name="PDNConfiguration"></a>

PDN is a utility that simplifies adding a power grid into the floorplan. With specifications given in the PDN configuration file, like which layer to use, stripe width and spacing, the utility can generate the metal straps used for the power grid. To create and configure a power grid, refer to [OpenROAD documentation](https://openroad.readthedocs.io/_/downloads/en/latest/pdf/) on pdngen. Following is a generic PDN configuration file:


```
# pdn.cfg
###############################
# POWER or GROUND #Std. cell rails starting with power or ground rails at the bottom of the core area
set ::rails_start_with "POWER" ;

# POWER or GROUND #Upper Mal stripes starting with power or ground rails at the left/bottom of the core area
set ::stripes_start_with "POWER" ;

# Power nets
set ::power_nets "VDD"
set ::ground_nets "VSS"

set pdngen::global_connections {
VDD {
  {inst_name .* pin_name VPWR}
  {inst_name .* pin_name VPB}
}
VSS {
  {inst_name .* pin_name VGND}
  {inst_name .* pin_name VNB}
}
}
#===> Power grid strategy
# Ensure pitches and offsets will make the stripes fall on track
pdngen::specify_grid stdcell {
  name grid
  rails {
      M1 {width 1.04 offset 0}
  }
  straps {
      M4 {width 2.08 pitch 20.28 offset 0}
      M5 {width 2.08 pitch 20.28 offset 0}
  }
  connect {{M1 M4} {M4 M5}}
}

pdngen::specify_grid macro {
  orient {R0 R180 MX MY}
  power_pins "VPWR"
  ground_pins "VGND"
  blockages "M1 M2 M3 M4"
  connect {{M4_PIN_ver M5}}
}

pdngen::specify_grid macro {
  orient {R90 R270 MXR90 MYR90}
  power_pins "VPWR"
  ground_pins "VGND"
  blockages "M1 M2 M3 M4"
  connect {{M4_PIN_hor M5}}
}
```



## Tapcell Configuration <a name="TapcellConfiguration"></a>

The tapcell configuration file is used to insert tapcells and endcaps into the design. The tapcell command has the following list of options:


```
    tapcell [-tapcell_master tapcell_master]
    [-endcap_master endcap_master]
    [-distance dist]
    [-halo_width_x halo_x]
    [-halo_width_y halo_y]
    [-tap_nwin2_master tap_nwin2_master]
    [-tap_nwin3_master tap_nwin3_master]
    [-tap_nwout2_master tap_nwout2_master]
    [-tap_nwout3_master tap_nwout3_master]
    [-tap_nwintie_master tap_nwintie_master]
    [-tap_nwouttie_master tap_nwouttie_master]
    [-cnrcap_nwin_master cnrcap_nwin_master]
    [-cnrcap_nwout_master cnrcap_nwout_master]
    [-incnrcap_nwin_master incnrcap_nwin_master]
    [-incnrcap_nwout_master incnrcap_nwout_master]
    [-tap_prefix tap_prefix]
    [-endcap_prefix endcap_prefix]
```


A simple, yet effective `tapcell.tcl` file is given below:


```
# tapcell.tcl
###################
tapcell \
  -endcap_cpp "2" \
  -distance 120 \
  -tapcell_master "FILL1" \
  -endcap_master "FILL1"
```



## setRC Configuration <a name="setRCConfiguration"></a>


setRC allows the user to define resistances and capacitances for layers and vias using the `set_layer_rc` command.. This is useful when these values are missing from the tech file. There is also a command that allows you to set the resistance and capacitance of routing wires using the `set_wire_rc`. Following is a generic example of a setRC configuration file which sets the resistance and capacitance of five metal layers, 4 vias, one signal wire, and one clock wire.


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



## KLayout <a name="KLayout"></a>

KLayout is used in OpenROAD to provide GDS merging, DRC, and LVS. Two files are required for KLayout and they are generated within the KLayout GUI. Install KLayout on the host machine since it is not included in the OpenROAD build process. Then create the properties and tech files as explained below.


## KLayout properties file <a name="KLayoutpropertiesfile"></a>

Follow these steps to generate the  KLayout properties file:



	1. Open KLayout
	2. Install the tf_import package
	    1. Inside KLayout, go to Tools
	    2. Manage Packages
	    3. Install New Packages
	    4. Select tf_import
	        1. If the source of the package is from github, then the file “” needs to be edited to include “source stdio”
	3. Re-start KLayout
	4. File -> Import some LEF. Doesn't matter what; you will just get an error message without one.
		1. Once selected, go to Options at the bottom left
		2. Select your layer map file under the Production tab
		3. Go to the LEF+Macro Files tab, then add under Additional LEF files, the merged (original) lef file in your platform directory
		4. Under Macro Layout Files, add the gds file in your platform directory
	5. File -> Import Cadence tech file
	    1. You have to select a tech file (found in the PDK, usually inside the Virtuoso folder)
	    2. KLayout also needs a .drf file which is automatically included if it resides in the same directory the cadence tech file was found in (found in the PDK’s Virtuoso folder).
	6. File -> Save Layer Properties
    	1. Save as a .lyp file in your platform directory


## KLayout tech file <a name="KLayouttechfile"></a>

Follow these steps to generate the Klayout tech file:



    1. Open klayout in a terminal
    2. Go to Tools -> Manage Technologies
    3. Click the + in the bottom left corner to create a new technology
    4. Set the name for the technology in the box that pops up. You should now see the technology name in the list on the left hand side
    5. Expand the technology by hitting the arrow and click on General
    6. Set the base path your platform directory and load the .lyp layer properties file that was generated earlier
    7. On the left hand side under your new technology click Reader Options and then click LEF/DEF on the top bar
    8. In the LEF+Macro Files section, you add the LEF files by clicking the + button on the right hand side of the box
        1. Only add your original merged lef file. Make sure it includes the full path to the lef file
    9. In the Production section, scroll down and add the layer map file by hitting the Load File button
        2. Make sure it includes the full path
    10. Above in the same section, change the layer name suffix and GDS data type to correspond with the layer map
    11. Generate the .lyt file by right clicking on the new technology name and click on Export Technology
        3. Save with the extension .lyt


# Validating the New Platform <a name="ValidatingPlatform"><a>

To validate the new platform, simply run a design through the flow using the new platform. The Makefile should already include the `DESIGN_CONFIG` variables for the new platform which were generated in the Setup section of the document. Simply uncomment a `DESIGN_CONFIG` variable for the new platform in the Makefile, save, and then run `make` in the terminal to run the design through the flow. Try a small design first (i.e. gcd) so that run time is small and you can identify and fix errors faster.


# Authors/Contributors <a name="AuthorsContributors"></a>



* James Stine - Oklahoma State University
* Teo Ene
* Ricardo Hernandez
* Ryan Ridley
* Indira Iyer
