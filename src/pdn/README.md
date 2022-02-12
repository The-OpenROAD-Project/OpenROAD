# PDNGEN

This utility aims to simplify the process of adding a power grid into a
floorplan. The aim is to specify a small set of power grid policies to be
applied to the design, such as layers to use, stripe width and spacing,
then have the utility generate the actual metal straps. Grid policies can
be defined over the stdcell area, and over areas occupied by macros.

## Commands

### Global Connections

#### Add global connections

The `add_global_connection` command is used to specify how to connect power and ground pins on design instances to the appropriate supplies.

```
add_global_connection -net net_name \
                      [-inst_pattern inst_regular_expression] \
                      -pin_pattern pin_regular_expression \
                      (-power|-ground) \
                      [-defer_connection]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | Specifies the name of the net in the design to which connections are to be added |
| `-inst_pattern` | Optional specifies a regular expression to select a set of instances from the design. (Default: .\*) |
| `-pin_pattern` | Species a regular expression to select pins on the selected instances to connect to the specified net |
| `-power` | Specifies that the net it a power net |
| `-ground` | Specifies that the net is a ground net |
| `-defer_connection` | Only add the connection, but wait to perform the connections when `global_connect` is run |

##### Examples
```
# Stdcell power/ground pins
add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

# RAM power ground pins
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSSE$}

```


#### Perform global connections

The `global_connect` command is used to connect power and ground pins on design instances to the appropriate supplies.

```
global_connect
```


### Define voltage domains

Defines a named voltage domain with the names of the power and ground nets for a region.

The -region argument specifies the name of a region of the design. This region must already exist in the floorplan before referencing it with the set_voltage_domain command. If the -region argument is not supplied then region is the entire extent of the design.

The -name argument is used to define a name for the voltage domain that can be used in the `define_pdn_grid` command
The -power and -ground arguments are used to define the names of the nets to be use for power and ground respectively within this voltage domain.

```
set_voltage_domain [-name name] \
                   -power power_net \
                   -ground ground_net \
                   [-region region_name] \
                   [-secondary_power secondary_power_net]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| -name | Defines the name of the voltage domain, default is "Core" or region name if provided |
| -power | Specifies the name of the power net for this voltage domain |
| -ground | Specifies the name of the ground net for this voltage domain |
| -region | Specifies a region of the design occupied by this voltage domain |
| -secondary_power | Specifies the name of the secondary power net for this voltage domain |

##### Examples
```
set_voltage_domain -power VDD -ground VSS
set_voltage_domain -name TEMP_ANALOG -region TEMP_ANALOG -power VIN -ground VSS
set_voltage_domain -region test_domain -power VDD -ground VSS -secondary_power VREG
```

### Define power grids

Define the rules to describe a power grid pattern to be placed in the design.

#### Define a core power grid

```
define_pdn_grid [-name <name>] \
                [-pins <list_of_pin_layers>] \
                [-starts_with (POWER|GROUND)] \
                [-voltage_domain <list_of_domain_names>] \
                [-starts_with (POWER|GROUND)]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | Defines a name to use when referring to this grid definition. |
| `-voltage_domain` | Defines the name of the voltage domain for this grid. (Default: Last domain created) |
| `-pins` | Defines a list of layers which where the power straps will be promoted to block pins. |
| `-starts_with` | Specifies whether the first strap placed will be POWER or GROUND (Default: POWER) |

##### Examples

```
define_pdn_grid -name main_grid -pins {metal7} -voltage_domain {CORE TEMP_ANALOG}
```


#### Define a macro power grid

```
define_pdn_grid -macro \
                [-name name] \
                [-grid_over_pg_pins|-grid_over_boundary] \
                [-orient <list_of_valid_orientations>] \
                [-instances <list_of_instances] \
                [-cells <list_of_cells>] \
                [-halo <list_of_halo_values>] \
                [-voltage_domain <list_of_domain_names>] \
                [-starts_with (POWER|GROUND)]    
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-macro` | Defines the type of grid being added as a macro. |
| `-name` | Defines a name to use when referring to this grid definition. |
| `-voltage_domain` | Defines the name of the voltage domain for this grid. (Default: Last domain created) |
| `-starts_with` | Specifies whether the first strap placed will be POWER or GROUND (Default: POWER) |
| `-grid_over_boundary` | Place the power grid over the entire macro. |
| `-grid_over_pg_pins` | Place the power grid over the power ground pins of the macro. (Default) |
| `-instances` | For a macro, defines a set of valid instances. Macros with a matching instance name will use this grid specification. |
| `-cells` | For a macro, defines a set of valid cells. Macros which are instances of one of these cells will use this grid specification. |
| `-orient` | For a macro, defines a set of valid orientations. LEF orientations (N, FN, S, FS, E, FE, W and FW) can be used as well as standard geometry orientations (R0, R90, R180, R270, MX, MY, MXR90 and MYR90). Macros with one of the valid orientations will use this grid specification. |
| `-halo` | Specifies the default minimum separation of selected macros from other cells in the design. This is only used if the macro does not define halo values in the LEF description. If 1 value is specified it will be used on all 4 sides, if two values are specified, the first will be applied to left/right sides and the second will be applied to top/bottom sides, if 4 values are specified, then they are applied to left, bottom, right and top sides respectively. (Default: 0) |

##### Examples

```
define_pdn_grid -macro -name ram          -orient {R0 R180 MX MY} -grid_over_pg_pins  -starts_with POWER -pin_direction vertical
define_pdn_grid -macro -name rotated_rams -orient {E FE W FW}     -grid_over_boundary -starts_with POWER -pin_direction horizontal
```

#### Add straps / stripes

Defines a pattern of power and ground stripes in a single layer to be added to a power grid.

```
add_pdn_stripe [-grid grid_name] \
                -layer layer_name \
                -width width_value \
                [-pitch pitch_value] \
                [-spacing spacing_value] \
                [-offset offset_value] \
                [-starts_with (POWER|GROUND)] \
                [-followpins] \
                [-extend_to_boundary] \
                [-extend_to_core_ring] \
                [-snap_to_grid] \
                [-number_of_straps count]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-grid` | Specifies the grid to which this stripe definition will be added. (Default: Last grid defined by `define_pdn_grid`) |
| `-layer` | Specifies the name of the layer for these stripes |
| `-width` | Value for the width of stripe |
| `-pitch` | Value for the distance between each power/ground pair |
| `-spacing` | Optional specification of the spacing between power/ground pairs within a single pitch. (Default: pitch / 2) |
| `-offset` | Value for the offset of the stripe from the lower left corner of the design core area. |
| `-starts_with` | Specifies whether the first strap placed will be POWER or GROUND (Default: POWER) |
| `-followpins` | Indicates that the stripe forms part of the stdcell rails, pitch and spacing are dictated by the stdcell rows, the `-width` is not needed if it can be determined from the cells |
| `-extend_to_boundary` | Extend the stripes to the boundary of the grid |
| `-snap_to_grid` | Snap the stripes to the defined routing grid |
| `-number_of_straps` | Number of power/ground pairs to add |

##### Examples
```
add_pdn_stripe -grid main_grid -layer metal1 -followpins
add_pdn_stripe -grid main_grid -layer metal2 -width 0.17 -followpins
add pdn_stripe -grid main_grid -layer metal4 -width 0.48 -pitch 56.0 -offset 2 -starts_with GROUND
```

#### Add rings

The `add_pnd_ring` command is used to define power/ground rings around a grid region. The ring structure is built using two layers that are orthogonal to each other. A power/ground pair will be added above and below the grid using the horizontal layer, with another power/ground pair to the left and right using the vertical layer. Together these 4 pairs of power/ground stripes form a ring around the specified grid. Power straps on these layers that are inside the enclosed region are extend to connect to the ring.

```
add_pdn_ring [-grid grid_name] \
             -layers layer_name \
             -widths (width_value|list_of_2_values) \
             -spacings (spacing_value|list_of_2_values) \
             [-core_offset offset_value] \
             [-pad_offset offset_value] \
             [-add_connect] \
             [-extend_to_boundary] \
             [-connect_to_pads] \
             [-connect_to_pad_layers layers]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-grid` | Specifies the name of the grid to which this ring defintion will be added. (Default: Last grid created by defin_pdn_grid)|
| `-layer` | Specifies the name of the layer for these stripes |
| `-width` | Value for the width of the stdcell rail |
| `-spacing` | Optional specification of the spacing between power/ground pairs within a single pitch. (Default: pitch / 2) |
| `-core_offset` | Value for the offset of the ring from the grid region |
| `-pad_offset` | When defining a power grid for the top level of an SoC, can be used to define the offset of ring from the pad cells |
| `-add_connect` | Automatically add a connection between the two layers |
| `-extend_to_boundary` | Extend the rings to the grid boundary |
| `-connect_to_pads` | The core side of the pad pins will be connected to the ring |
| `-connect_to_pad_layers` | Restrict the pad pins layers to this list |

##### Examples
```
add_pdn_ring -grid main_grid -layer {metal6 metal7} -widths 5.0 -spacings  3.0 -core_offset 5
```

#### Add connections

The `add_pdn_connect` command is used to define which layers in the power grid are to be connected together. During power grid generation, vias will be added for overlapping power nets and overlapping ground nets. The use of fixed vias from the technology file can be specified or else via stacks will be constructed using VIARULEs. If VIARULEs are not available in the technology, then fixed vias must be used.

```
add_pdn_connect [-grid grid_name] \
                [-layers list_of_two_layers] \
                [-cut_pitch pitch_value] \
                [-fixed_vias list_of_fixed_vias]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-grid` | Specifies the name of the grid definition to which this connection will be added. (Default: Last grid created by `define_pdn_grid`) |
| `-layers` | Layers to be connected where there are overlapping power or overlapping ground nets |
| `-cut_pitch` | When the two layers are parallel e.g. overlapping stdcell rails, specify the distance between via cuts |
| `-fixed_vias` | list of fixed vias to be used to form the via stack |

##### Examples

```
add_pdn_connect -grid main_grid -layers {metal1 metal2} -cut_pitch 0.16
add_pdn_connect -grid main_grid -layers {metal2 metal4}
add_pdn_connect -grid main_grid -layers {metal4 metal7}

add_pdn_connect -grid ram -layers {metal4 metal5}
add_pdn_connect -grid ram -layers {metal5 metal6}
add_pdn_connect -grid ram -layers {metal6 metal7}

add_pdn_connect -grid rotated_rams -layers {metal4 metal6}
add_pdn_connect -grid rotated_rams -layers {metal6 metal7}
```

### Build power grid

Build a power grid in accordance with the information specified.

```
pdngen [-skip_trim] \
       [-dont_add_pins] \
       [-reset] \
       [-ripup] \
       [-report_only]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-skip_trim` | Skip the metal trim step, which attempts to remove metal stubs |
| `-dont_add_pins` | Prevent the creation of block pins during |
| `-reset` | Reset the grid and domain specifications |
| `-ripup` | Ripup the existing power grid, as specified by the voltage domains |
| `-report_only` | Print the current specifications |

### Converting former PDNGEN configuration file to tcl commands

To get an initial conversion from the former PDNGEN configuration file to the current tcl commands use `convert_pdn_config`.
This command will provide an initial set of commands based on the provided file, it is recommended that the user double-check
the conversion to ensure nothing was missed.

```
convert_pdn_config config_file
```

##### Options

| Name | Description |
| ----- | ----- |
| `config_file` | Path to the old configuration file |


## Example scripts

### Defining a SoC power grid with pads

```
add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_ring -grid "Core" -layers {metal8 metal9} -widths 5.0 -spacings 2.0 -core_offsets 4.5 -connect_to_pads

add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

add_pdn_stripe -layer metal4 -width 0.48 -pitch 56.0 -offset 2.0 -extend_to_core_ring
add_pdn_stripe -layer metal7 -width 1.40 -pitch 40.0 -offset 2.0 -extend_to_core_ring
add_pdn_stripe -layer metal8 -width 1.40 -pitch 40.0 -offset 2.0 -extend_to_core_ring
add_pdn_stripe -layer metal9 -width 1.40 -pitch 40.0 -offset 2.0 -extend_to_core_ring

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}
add_pdn_connect -layers {metal7 metal8}
add_pdn_connect -layers {metal8 metal9}
add_pdn_connect -layers {metal9 metal10}

pdngen
```

## Regression tests

## Limitations

Currently the following assumptions are made:

1. The design is rectangular
1. The input floorplan includes the stdcell rows, placement of all macro blocks and IO pins.
1. The stdcells rows will be cut around macro placements

## FAQs

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
