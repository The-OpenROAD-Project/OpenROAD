# OpenROAD-pdn

This utility aims to simplify the process of adding a power grid into a floorplan.
The aim is to specify a small set of power grid policies to be applied to the design, such as layers to use,
stripe width and spacing, then have the utility generate the actual metal straps. Grid policies can be defined
over the stdcell area, and over areas occupied by macros.

For more details refer to [doc/README.md](doc/README.md)


#### Power distribution network

The creation of a power distribution network is done using pdngen.
The [add_global_connection](doc/add_global_connection.md) command is used to connect cells to the power/ground nets

```
add_global_connection \
    -net net_name \
    [-inst_pattern inst_regular_expression] \
    -pin_pattern pin_regular_expression
```

The [define_pdn_grid](src/pdn/define_pdn_grid.md) command is used to define the layers to be used, along with their widths and spacings, to build the power grid.

For specifying a power grid over the stdcell area
```
define_pdn_grid
    -type stdcell \
    [-name name] \
    [-rails list_of_rail_specifications] \
    [-straps list_of_strap_specifications] \
    [-pins list_of_pin_layers] \
    [-connect list_of_connected_layer_pairs] \
    [-starts_with (POWER|GROUND)]
```
For specifying a power grid over macros in the design
```
define_pdn_grid
    -type macro \
    [-name name] \
    [-straps list_of_strap_specifications] \
    [-connect list_of_connected_layer_pairs] \
    [-blockages list_of_blocked_layers] \
    [-orient list_of_valid_orientations>] \
    [-power_pins list_of_power_pin_names] \
    [-ground_pins list_of_groiund_pin_names] \
    [-connect list_of_connected_layer_pairs] \
    [-starts_with (POWER|GROUND)]
```

Once the power grid is defined the [pdngen](src/pdn/doc/pdngen.md) command is used to build the required power grid
```
pdngen [-verbose]
```
