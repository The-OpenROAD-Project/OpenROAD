## set_padring_options
### Synopsis
```
  % set_padring_options \
      -type (flipchip|wirebond) \
      [-power power_nets] \
      [-ground ground_nets] \
      [-offsets offsets] \
      [-pad_inst_name pad_inst_name] \
      [-pad_pin_name pad_pin_name] \
      [-pin_layer pin_layer_name] \
      [-connect_by_abutment signal_list] \
      [-rdl_cover_file_name rdl_file_name]
```
### Description

This command is used to specify general options used to define the padring for a chip. The -type option is required, all others are optional.

The -power and -ground options are used to define the external power and ground nets that will be connected to the pads.

The -offsets option is used to define the offset of the edge of the pads from the edge of the die area, this can be specified as a single number, in which case the offst is applied to all four sides, as a list of 2 numbers to be applied to the top/bottom and lef/right edges, or a set of 4 numbers to be applied to the bottom, right, top and left sides respectively.

The -pad_inst_name option allows a format string to be used to define a default instance name for the padcell connected to a top level pin of the design.

The -pad_pin_name option is used to define the default value for the name of the pin on the padcells that is to be connected to the top level pin of the design.

The -connect_by_abutment option is used to define the list of signals that are connected by abutment through the padring. The placement of breaker cells within the padring can result in these signals being split into a number of different nets.

The -rdl_cover_file_name is used to specify the name of the file to contain the RDL routing for a flipchip design.

### Options

| Option | Description |
| --- | --- |
| -type | Specify whether the chip is wirebond or flipchip |
|  -power | Define the nets to be used as power nets in the design, it is not required that these nets exist in the design database, they will be created as necessary. Once the power net is defined, it can be used as the -signal argument for the ```add_pad``` command to enable the addition of power pads to the design. |
| -ground | Define the nets to be used as ground nets in the design, it is not required that these nets exist in the design database, they will be created as necessary. Once a ground net is defined, it can be used as the -signal argument for the ```add_pad``` command to enable the addition of ground pads to the design. |
| -core_area | Specify the co-ordinates, as a list of 4 numbers (in micrometres), of the chip core. This area is reserved for stdcell placements. Placement of padcells should be made outside this core area, but within the specified die area |
| -die_area | Specify the co-ordinates, as a list of 4 numbers (in micrometres), for the chip die area. |
| -offsets | Specifies the offset, in micrometres, from the edge of the die area to the edge of the padcells. |
| -pad_inst_name | Specify the name of padcell instances based upon the given format string. The format string is expected to include %s somewhere within the string, which will be replaced by the signal name associated with each pad instance. |
| -pad_pin_name | Used to set the name of the pin on a padcell which is to be connected to the specified signal for the padcell. |
| -pin_layer | Specify the layer which is to be used to create a top level pin over the pin of the padcell. The creation of a physical pin at this location identifies pin locations for LVS. |
| -connect_by_abutment | Specify the list of signals that connect by abutment in the padring, the placement of breaker cells in the padring will split these signals into separate nets as required. |
| -rdl_cover_file_name | Specify the name of the file to which the routing of the redistribution layer is to be written. If not specified, the default value is cover.def.  In the previous release, the openroad database did not support 45 degree lines used by RDL routing, and this cover.def allowed for the RDL to be added at the end of the flow, without being added to the database. Now that the database will allow 45 degree lines, and this command will be deprecated once ICeWall has been modified to write RDL into the database directly. |

### Examples
```
set_padring_options \
  -type wirebond \
  -power  {VDD DVDD_0 DVDD_1} \
  -ground {VSS DVSS_0 DVSS_1} \
  -offsets 35 \
  -pin_layer metal10 \
  -pad_inst_name "%s" \
  -pad_pin_name "PAD" \
  -connect_by_abutment {SNS RETN DVDD DVSS}

```

