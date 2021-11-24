# set_padring_options
## Synopsis
```
  % set_padring_options \
      -type (flipchip|wirebond) \
      [-power power_nets] \
      [-ground ground_nets] \
      [-offsets offsets] \
      [-pad_inst_pattern pad_inst_pattern] \
      [-pad_pin_pattern pad_pin_pattern] \
      [-pin_layer pin_layer_name] \
      [-connect_by_abutment signal_list]
```
## Description

This command is used to specify general options used to define the padring for a chip. The -type option is required; all others are optional.

The -power and -ground options are used to define the external power and ground nets that will be connected to the pads.

The -offsets option is used to define the offset of the edge of the pads from the edge of the die area. This can be specified as a single number, in which case the offset is applied to all four sides, or as a list of 2 numbers that are respectively applied to the top/bottom and left/right edges, or as a list of 4 numbers that are respectively applied to the bottom, right, top and left edges.

The -pad_inst_pattern option allows a format string to be used to define a default instance name for the padcell connected to a top-level pin of the design.

The -pad_pin_pattern option is used to define a pattern that is to be used to derive the actual signal name in the design from the signal specified with the add_pad command.

The -connect_by_abutment option is used to define the list of signals that are connected by abutment through the padring. The placement of breaker cells within the padring can result in these signals being split into a number of different nets.


## Options

| Option | Description |
| --- | --- |
| -type | Specify whether the chip is wirebond or flipchip. |
|  -power | Define the nets to be used as power nets in the design. It is not required that these nets exist in the design database; they will be created as necessary. Once a power net is defined, it can be used as the -signal argument for the ```add_pad``` command to enable the addition of power pads to the design. |
| -ground | Define the nets to be used as ground nets in the design, it is not required that these nets exist in the design database, they will be created as necessary. Once a ground net is defined, it can be used as the -signal argument for the ```add_pad``` command to enable the addition of ground pads to the design. |
| -core_area | Specify the coordinates, as a list of 4 numbers (in microns), of the chip core. This area is reserved for stdcell placements. Placement of padcells should be made outside this core area, but within the specified die area. |
| -die_area | Specify the coordinates, as a list of 4 numbers (in microns), for the chip die area. |
| -offsets | Specifies the offset, in microns, from the edges of the die area to corresponding edges of the padcells. |
| -pad_inst_pattern | Specify the name of padcell instances based upon the given format string. The format string is expected to include %s somewhere within the string, which will be replaced by the signal name associated with each pad instance. |
| -pad_pin_pattern | Specify the name of the signal to connect to the padcell based upon the given format string. The format string is expected to include %s somewhere within the string, which will be replaced by the signal name associated with the pad instance. |
| -pin_layer | Specify the layer which is to be used to create a top level pin over the pin of the padcell. The creation of a physical pin at this location identifies pin locations for LVS (layout-versus-schematic) verification. |
| -connect_by_abutment | Specify the list of signals that connect by abutment in the padring. The placement of breaker cells in the padring will split these signals into separate nets as required. |

## Examples
```
set_padring_options \
  -type wirebond \
  -power  {VDD DVDD_0 DVDD_1} \
  -ground {VSS DVSS_0 DVSS_1} \
  -offsets 35 \
  -pad_inst_pattern "u_%s" \
  -pad_pin_pattern "p_%s" \
  -pin_layer metal10 \
  -connect_by_abutment {SNS RETN DVDD DVSS}

```

