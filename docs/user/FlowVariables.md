# Environment Variables for the OpenROAD Flow Scripts


Environment variables are used in the OpenROAD flow to define various
platform, design and tool specific variables to allow finer control and
user overrides at various flow stages. These are defined in the
`config.mk` file located in the platform and design specific directories.


## Platform


These variables must be defined in the platform specific config file
located in the OpenROAD-flow-scripts directory of
`./flow/platforms/<PLATFORM>/config.mk`


Note: The variable `PLATFORM_DIR` is set by default based on the `PLATFORM`
variable. For OpenROAD Flow Scripts we have the following public platforms:


-   `sky130hd`
-   `sky130hs`
-   `nangate45`
-   `asap7`


## Platform Specific Environment Variables


The table below lists the complete set of variables used in each of the
public platforms supported by the OpenROAD flow.


Note:
-   = indicates default definition assigned by the tool
-   ?= indicates that the variable value may be reassigned with design `config.mk`
-   N/A indicates that the variable/files is not supported currently.


| **Configuration Variable**           | **sky130hd** | **sky130hs** | **nangate45** | **asap7** |
|--------------------------------------|--------------|--------------|---------------|-----------|
| Library Setup                        |              |              |               |           |
| `PROCESS`                            | =            | =            | =             | =         |
| `TECH_LEF`                           | =            | =            | =             | =         |
| `SC_LEF`                             | =            | =            | =             | =         |
| `LIB_FILES`                          | =            | =            | =             | =         |
| `GDS_FILES`                          | =            | =            | =             | =         |
| `DONT_USE_CELLS`                     | =            | =            | =             | =         |
| Synthesis                            |              |              |               |           |
| `LATCH_MAP_FILE`                     | =            | =            | =             | =         |
| `CLKGATE_MAP_FILE`                   | =            | =            | =             | =         |
| `ADDER_MAP_FILE`                     | ?=           | ?=           | ?=            | ?=        |
| `TIEHI_CELL_AND_PORT`                | =            | =            | =             | =         |
| `TIELO_CELL_AND_PORT`                | =            | =            | =             | =         |
| `MIN_BUF_CELL_AND_PORTS`             | =            | =            | =             | =         |
| `ABC_CLOCK_PERIOD_IN_PS`             | ?=           | ?=           | ?=            | ?=        |
| `ABC_DRIVER_CELL`                    | =            | =            | =             | =         |
| `ABC_LOAD_IN_FF`                     | =            | =            | =             | =         |
| Floorplan                            |              |              |               |           |
| `PLACE_SITE`                         | =            | =            | =             | =         |
| `MAKE_TRACKS`                        | =            | =            | =             | =         |
| `TAPCELL_TCL`                        | =            | =            | =             | =         |
| `MACRO_PLACE_HALO`                   | ?=           | ?=           | ?=            | ?=        |
| `MACRO_PLACE_CHANNEL`                | ?=           | ?=           | ?=            | ?=        |
| `PDN_CFG`                            | ?=           | ?=           | ?=            | ?=        |
| `IO_PLACER_H`                        | =            | =            | =             | =         |
| `IO_PLACER_V`                        | =            | =            | =             | =         |
| Placement                            |              |              |               |           |
| `CELL_PAD_IN_SITES_GLOBAL_PLACEMENT` | ?=           | ?=           | ?=            | ?=        |
| `CELL_PAD_IN_SITES_DETAIL_PLACEMENT` | ?=           | ?=           | ?=            | ?=        |
| `PLACE_DENSITY`                      | ?=           | ?=           | ?=            | ?=        |
| `WIRE_RC_LAYER`                      | =            | =            | =             | =         |
| Clock Tree Synthesis                 |              |              |               |           |
| `CTS_BUF_CELL`                       | =            | =            | =             | =         |
| `FILL_CELLS`                         | =            | =            | =             | =         |
| `CTS_BUF_DISTANCE`                   | N/A          | N/A          | N/A           | =         |
| Routing                              |              |              |               |           |
| `MIN_ROUTING_LAYER`                  | =            | =            | =             | =         |
| `MAX_ROUTING_LAYER`                  | =            | =            | =             | =         |
| `FASTROUTE_TCL`                      | ?=           | ?=           | ?=            | N/A       |
| `RCX_RULES`                          | =            | =            | =             | =         |
| `KLAYOUT_TECH_FILE`                  | =            | =            | =             | =         |
| `FILL_CONFIG`                        | =            | =            | N/A           | N/A       |


### Library Setup


| Variable         | Description                                                                                                                                          |
|------------------|------------------------------------------------------------------------------------------------------------------------------------------------------|
| `PROCESS`        | Technology node or process in use.                                                                                                                   |
| `TECH_LEF`       | A technology LEF file of the PDK that includes all relevant information regarding metal layers, vias, and spacing requirements.                      |
| `SC_LEF`         | Path to technology standard cell LEF file.                                                                                                           |
| `GDS_FILES`      | Path to platform GDS files.                                                                                                                          |
| `LIB_FILES`      | A Liberty file of the standard cell library with PVT characterization, input and output characteristics, timing and power definitions for each cell. |
| `DONT_USE_CELLS` | Dont use cells eases pin access in detailed routing.                                                                                                 |


### Synthesis


| Variable                 | Description                                                                                |
|--------------------------|--------------------------------------------------------------------------------------------|
| `LATCH_MAP_FILE`         | List of latches treated as a black box by Yosys.                                           |
| `CLKGATE_MAP_FILE`       | List of cells for gating clock treated as a black box by Yosys.                            |
| `ADDER_MAP_FILE`         | List of adders treated as a black box by Yosys.                                            |
| `TIEHI_CELL_AND_PORT`    | Tie high cells used in Yosys synthesis to replace a logical 1 in the Netlist.              |
| `TIELO_CELL_AND_PORT`    | Tie low cells used in Yosys synthesis to replace a logical 0 in the Netlist.               |
| `MIN_BUF_CELL_AND_PORTS` | Used to insert a buffer cell to pass through wires. Used in synthesis.                     |
| `ABC_CLOCK_PERIOD_IN_PS` | Clock period to be used by STA during synthesis. Default value read from `constraint.sdc`. |
| `ABC_DRIVER_CELL`        | Default driver cell used during ABC synthesis.                                             |
| `ABC_LOAD_IN_FF`         | During synthesis set_load value used.                                                      |


### Floorplan


| Variable              | Description                                                                                                                                                                      |
|-----------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `PLACE_SITE`          | Placement site for core cells defined in the technology LEF file.                                                                                                                |
| `TAPCELL_TCL`         | Path to Endcap and Welltie cells file.                                                                                                                                           |
| `MACRO_PLACE_HALO`    | horizontal/vertical halo around macros (microns).                                                                                                                                |
| `MACRO_PLACE_CHANNEL` | horizontal/vertical channel width between macros (microns).                                                                                                                      |
| `PDN_CFG`             | File path which has a set of power grid policies used by `pdn` to be applied to the design, such as layers to use, stripe width and spacing to generate the actual metal straps. |
| `MAKE_TRACKS`         | Tcl file that defines add routing tracks to a floorplan.                                                                                                                         |
| `IO_PLACER_H`         | The metal layer on which to place the I/O pins horizontally (top and bottom of the die).                                                                                         |
| `IO_PLACER_V`         | The metal layer on which to place the I/O pins vertically (sides of the die).                                                                                                    |


### Placement


| Variable                             | Description                                                                                                                                   |
|--------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| `CELL_PAD_IN_SITES_GLOBAL_PLACEMENT` | Cell padding on both sides in site widths to ease routability during global placement.                 		                       |
| `CELL_PAD_IN_SITES_DETAIL_PLACEMENT` | Cell padding on both sides in site widths to ease routability in detail placement.			                                       |
| `PLACE_DENSITY`                      | The desired placement density of cells. It reflects how spread the cells would be on the core area. 1.0 = closely dense. 0.0 = widely spread. |


### Clock Tree Synthesis(CTS)


| Variable       | Description                                                                                                   |
|----------------|---------------------------------------------------------------------------------------------------------------|
| `CTS_BUF_CELL` | The buffer cell used in the clock tree.                                                                       |
| `FILL_CELLS`   | Fill cells are used to fill empty sites.									 |


### Routing


| Variable            | Description                                         |
|---------------------|-----------------------------------------------------|
| `MIN_ROUTING_LAYER` | The lowest metal layer name to be used in routing.  |
| `MAX_ROUTING_LAYER` | The highest metal layer name to be used in routing. |


### Extraction


| Variable            | Description                                           |
|---------------------|-------------------------------------------------------|
| `RCX_RULES`         | RC Extraction rules file path.                        |
| `SET_RC_TCL`        | Metal & Via RC definition file path.                  |
| `FILL_CONFIG`       | JSON rule file for metal fill during chip finishing.  |
| `KLAYOUT_TECH_FILE` | A mapping from LEF/DEF to GDS using the KLayout tool. |


## Design Specific Configuration Variables


### Required Variables


Following minimum design variables must be defined in the design configuration
file for each design located in the OpenROAD-flow-scripts directory of
`./flow/designs/<PLATFORM>/<DESIGN_NAME>/config.mk`


| Variable        | Description                                                                                                                                                          |
|-----------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `PLATFORM`      | Specifies process design kit or technology node to be used.                                                                                                          |
| `DESIGN_NAME`   | The name of the top-level module of the design.                                                                                                                      |
| `VERILOG_FILES` | The path to the design Verilog files.                                                                                                                                |
| `SDC_FILE`      | The path to design constraint (SDC) file.                                                                                                                            |


### Optional Variables


These are optional variables that may be over-ridden/appended with
default value from the platform `config.mk` by defining in the design
configuration file.


#### Synthesis


| Variable                 | Description                                                                                        |
|--------------------------|----------------------------------------------------------------------------------------------------|
| `ADDITIONAL_LEFS`        | Hardened macro LEF view files listed here.                                                         |
| `ADDITIONAL_LIBS`        | Hardened macro library files listed here.                                                          |
| `ADDITIONAL_GDS`         | Hardened macro GDS files listed here.                                                              |
| `VERILOG_INCLUDE_DIRS`   | Specifies the include directories for the Verilog input files.                                     |
| `CORNER`                 | PVT corner library selection.                                                                      |
| `DESIGN_NICKNAME`        | DESIGN_NICKNAME just changes the directory name that ORFS outputs to be DESIGN_NICKNAME instead of DESIGN_NAME in case DESIGN_NAME is unwieldy or conflicts with a difference design.                                                                                    |
| `ABC_AREA`               | Strategies for Yosys ABC synthesis: Area/Speed. Default ABC_SPEED.                                 |
| `PWR_NETS_VOLTAGES`      | Used for IR Drop calculation.                                                                      |
| `GND_NETS_VOLTAGES`      | Used for IR Drop calculation.                                                                      |


#### Floorplan


| Variable              | Description                                                                                                                                                                                                                                |
|-----------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `CORE_UTILIZATION`    | The core utilization percentage (0-100). Overrides `DIE_AREA` and `CORE_AREA`.                                                                                                                                                                   |
| `CORE_ASPECT_RATIO`   | The core aspect ratio (height / width). This values is ignored if `CORE_UTILIZATION` undefined.                                                                                                                                            |
| `CORE_MARGIN`         | The margin between the core area and die area, in multiples of SITE heights. The margin is applied to each side. This variable is ignored if `CORE_UTILIZATION` is undefined.                                                              |
| `DIE_AREA`            | The die area specified as a list of lower-left and upper-right corners in microns (X1 Y1 X2 Y2). This variable is ignored if `CORE_UTILIZATION` and `CORE_ASPECT_RATIO` are defined.                                                       |
| `CORE_AREA`           | The core area specified as a list of lower-left and upper-right corners in microns (X1 Y1 X2 Y2). This variable is ignored if `CORE_UTILIZATION` and `CORE_ASPECT_RATIO` are defined.                                                      |

