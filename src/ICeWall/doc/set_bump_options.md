## set_bump_options

### Synopsis
```
  % set_bump_options \
        [-pitch pitch] \
        [-spacing_to_edge spacing] \
        [-offset {x_offset y_offset}] \
        [-array_size {num_rows num_cols}] \
        [-bump_pin_name pin_name] \
        [-cell_name bump_cell_table] \
        [-num_pads_per_tile value] \
        [-rdl_layer name] \
        [-rdl_width value] \
        [-rdl_spacing value] \
        [-rdl_cover_file_name rdl_file_name]
```

### Description
The set_bump_options command is used to provide detailed information about how to handle bumps and the redistribution layer (RDL) connections.

Use the -pitch options to set the centre-to-centre spacing of bumps.

If the -spacing_to_edge option is specified, then the number of rows and columns of bumps added will be the maximum that can fit in the die with a minimum spacing to the edge of the die as specified. Alternatively, specify the number of rows are columns of bump using the -array_size option, and specify the -offset option to specify the location of the lower left bump on the die.

The name of the cell in the library is specified with the -cell name option. If the technology supports a number of different bump cells according to bump pitch, then the value for cell name can be specified as a list of key value pairs, where the key is pitch between bump centres, and the value is the name of the bump cell to be used. The actual cell selected will depend upon the value of the -pitch option.

The name of the pin on the bump is specified with the -bump_pin_name

The -num_pads_per_tile options specifies the number of padcells that can be placed within a single bump pitch. This can be specified as an integer, or as a list of key value pairs using keys the same as for the -cell_name option.

Details about the redistribution layer, name, width and spacing are specified with the -rdl_layer, -rdl_width and -rdl_spacing commands (units: micrometres)

The -rdl_cover_file_name is used to specify the name of the file to contain the RDL routing.

### Options

| Option | Description |
| --- | --- |
| -pitch | Specifies the centre-to-centre spacing of bumps |
| -spacing_to_edge | Specifies the spacing from the edge of the die to the edge of the bumps |
| -array_size | Specifies the number of rows and columns of bumps as a 2 element list |
| -offset | Specifies the location of the centre of the lower left bump on the die |
| -bump_pin_name | The name of he pin on the bump cell |
| -cell_name | Specifies the name of the bump cell, or a list of key value pairs giving different values of bump cell name with the value of -pitch used as a key |
| -num_pads_per_tile | The maximum number of externally connected padcells placed within a bump pitch |
| -rdl_layer | Name of the redistribution layer |
| -rdl_width | The width of the RDL layer to use when connecting bumps to padcells |
| -rdl_spacing | The required spacing between RDL wires |
| -rdl_cover_file_name | Specify the name of the file to which the routing of the redistribution layer is to be written. If not specified, the default value is cover.def.  In the previous release, the openroad database did not support 45 degree lines used by RDL routing, and this cover.def allowed for the RDL to be added at the end of the flow, without being added to the database. Now that the database will allow 45 degree lines, and this command will be deprecated once ICeWall has been modified to write RDL into the database directly. |

### Examples
```
set_bump_options \
  -pitch 160 \
  -bump_pin_name PAD \
  -spacing_to_edge 165 \
  -cell_name {140 BUMP_small 150 BUMP_medium 180 BUMP_large} \
  -num_pads_per_tile 5 \
  -rdl_layer metal10 \
  -rdl_width 10 \
  -rdl_spacing 10


set_bump_options \
  -pitch 160 \
  -bump_pin_name PAD \
  -array_size {17 17} \
  -offset {210.0 215.0} \
  -cell_name DUMMY_BUMP \
  -num_pads_per_tile 5 \
  -rdl_layer metal10 \
  -rdl_width 10 \
  -rdl_spacing 10
```

