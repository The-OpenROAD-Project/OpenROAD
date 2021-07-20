# ICeWall

## Introduction

ICeWall is a utility to place IO cells around the periphery of a design, and associate the IO cells with those present in the netlist of the design.

Additional data is required to define the IO cells in the library over and above that specified in the LEF description. e.g.
* Define the orientation of IO cells on each side
* Allow different cell types to be used for the same type of signal depending upon which side of the die the IO cell is to be placed on. Technologies that require the POLY layer to be aligned in the same direction across the whole die require different cell variants for left/right edges and top/bottom edges
* Define the pins that connect through the sides of the pad cells by abutment
* Define cells that are used as breaker cells around the padring, defining the signals that they break.
* Define the filler cells and corner cells to be used.

In order to allow for a given padring to be reused across multiple designs, the definition of the placement of padcells is done independently of the signals associated with the IO cells. It is expected that the incoming design will instantiate IO cells for all IO cells that include discrete devices i.e. signal IO as well as power/ground pads. 

The package data file defines where padcells are to be placed, but the association between the placement and the netlist not made until the signal mapping file is read in. This separation of placement and netlist information allows the same IO placement to be re-used for multiple designs. A package data file can also be extracted from an existing DEF file to allow padrings of existing designs to be reused.

The signal mapping file associates signals in the netlist with locations in the padring

## Data definitions

ICeWall acts upon the information provided in the data files to produce a padring layout. The required data is split across 3 files to allow for the anticipated use models

### Library data

The library data is defined by calling the Footprint library function. This function takes a single argument, which is a TCL dictionary structure that defines the needed definitions of the cells.

The following keys need to be defined in the dictionary
* types
* connect_by_abutment
* pad_pin_name
* pad_pin_layer
* breakers
* cells

The types entry associates types used in the package data file with cells defined in the cells section, and in addition requires the definition of the corner cell to be used and the list of pad filler cells to be used ordered by cell width.

The connect_by_abutment key defines the list of pins on the edges of the IO cells that are connected by abutment forming a ring of these signals around the die. ICeWall will connect these abutted signals together into SPECIAL nets, so that the router will not try to add wiring for these pins.

The pad_pin_name key defines the name of the pin of the IO cell that is connected to the chip package.

The pad_pin_layer key defines the layer that will be used to create a physical pin on the die that is coincident with the location of the external connection to the chip package

The breakers key defines the list of cell types in the types definition that act as signal breakers for the signals that connect by abutment.

The cells key defines additional data for each IO cell. Each type defined in the types key must have a matching definition in the list of cells. Within a cell definition, the following keys may be used

* cell__name
* orient
* physical_only
* breaks
* signal_type 
* power_pad
* ground_pad
* pad_pin_name

| Key | Description |
| --- | --- |
| cell_name | Can either be the name of the LEF macro to use, or else a dictionary allowing a different cell name to be specified for each side of the die. Sides are denoted by bottom right top and left |
| orient | A dictionary that specifies the orientation of the cell for each side of the die |
| physical_only | Cells that are not associated with cells in the design, primarily used for physical connections through the pad ring |
| breaks | For breaker cells, defines the abutment signals which are broken by the presence of this cell. Each signal broken requires a list of pins on the breaker cell for the left and right hand side of the cell (An empty list may be provided if there are no pins for the broken signals |
| signal_type | Set to 1 if this cell is to be regarded as a signal type |
| power_pad | Optional. Set to 1 to define this cell as a power pad |
| ground_pad | Optional. Set to 1 o define this cell as a ground pad |
| pad_pin_name | Optional. Define the name of the pin on the macro that connects to the chip package |


### TCL Command Reference

Details on the [TCL command interface](TCL_Interface.md)


### Package data

As an alternative to using TCL commands, the definition of the padring can be done using a padring strategy file, which allows for batch loading of the padcell data all at once.

For an example of what a padring strategy file looks like refer to [../test/soc_bsg_black_parrot_nangate45/bsg_black_parrot.package.strategy](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/src/ICeWall/test/soc_bsg_black_parrot_nangate45/bsg_black_parrot.package.strategy).

### Signal mapping

Assignment of signals to padcells can be done via a signal mapping file. This file consists of multiple lines, each one mapping a named padcell to a signal in the design. 

For power and ground pads the same power/ground signal will likely be associated with multiple padcells.

#### Example
```
sig132   ddr_dq_10_io
sig133   ddr_dq_9_io
sig134   ddr_dq_8_io
v18_0    DVDD_0
v18_1    DVDD_0
v18_2    DVDD_0
v18_3    DVDD_0
```
