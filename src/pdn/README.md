# PDNGEN

This utility aims to simplify the process of adding a power grid into a
floorplan. The aim is to specify a small set of power grid policies to be
applied to the design, such as layers to use, stripe width and spacing,
then have the utility generate the actual metal straps. Grid policies can
be defined over the stdcell area, and over areas occupied by macros.

## Commands

### Define Power Switch Cell

Define a power switch cell that will be inserted into a power grid 
```
define_power_switch_cell -name <name> \
                         -control <control_pin_name> \
                         [-acknowledge <acknowledge_pin_name>] \
                         -switched_power <switched_power_pin> \
                         -power <unswitched_power_pin> \
                         -ground <ground_pin> 
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | The name of the power switch cell. |
| `-control` | The name of the power control port of the power switch cell. |
| `-acknowledge` | Defines the name of the output control signal of the power control switch if it has one. |
| `-switched_power` | Defines the name of the pin that outputs the switched power net |
| `-power` | Defines the name of the pin that connects to the unswitched power net. |
| `-ground` | Defines the name of the pin that connects to the ground net. |

#### Examples
```
define_power_switch_cell -name POWER_SWITCH -control SLEEP -switched_power VDD -power VDDG -ground VSS

```

### Define voltage domains

Defines a named voltage domain with the names of the power and ground nets for a region.

The -region argument specifies the name of a region of the design. This region must already exist in the floorplan before referencing it with the set_voltage_domain command. If the -region argument is not supplied then region is the entire extent of the design.

The -name argument is used to define a name for the voltage domain that can be used in the `define_pdn_grid` command
The -power and -ground arguments are used to define the names of the nets to be use for power and ground respectively within this voltage domain.

If the voltage domain is a switched power domain, then the name of the swiched power net must be specified with the -switched_power option.
```
set_voltage_domain [-name name] \
                   -power power_net \
                   -ground ground_net \
                   [-region region_name] \
                   [-secondary_power secondary_power_net] \
                   [-switched_power <switched_power_net>]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | Defines the name of the voltage domain, default is "Core" or region name if provided |
| `-power` | Specifies the name of the power net for this voltage domain |
| `-ground` | Specifies the name of the ground net for this voltage domain |
| `-region` | Specifies a region of the design occupied by this voltage domain |
| `-secondary_power` | Specifies the name of the secondary power net for this voltage domain |
| `-switched_power` | Specifies the name of the switched power net for switched power domains, |

##### Examples
```
set_voltage_domain -power VDD -ground VSS
set_voltage_domain -name TEMP_ANALOG -region TEMP_ANALOG -power VIN -ground VSS
set_voltage_domain -region test_domain -power VDD -ground VSS -secondary_power VREG
```

### Define power grids

Define the rules to describe a power grid pattern to be placed in the design.

```
define_pdn_grid [-name <name>] \
                [-pins <list_of_pin_layers>] \
                [-starts_with (POWER|GROUND)] \
                [-voltage_domain <list_of_domain_names>] \
                [-starts_with (POWER|GROUND)] \
                [-obstructions <list_of_layers>]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | Defines a name to use when referring to this grid definition. |
| `-voltage_domain` | Defines the name of the voltage domain for this grid. (Default: Last domain created) |
| `-pins` | Defines a list of layers which where the power straps will be promoted to block pins. |
| `-starts_with` | Specifies whether the first strap placed will be POWER or GROUND (Default: GROUND) |
| `-obstructions` | Specify the layers to add routing blockages, in order to avoid DRC violations |


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
                [-default] \
                [-halo <list_of_halo_values>] \
                [-voltage_domain <list_of_domain_names>] \
                [-starts_with (POWER|GROUND)] \
                [-obstructions <list_of_layers>]  
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-macro` | Defines the type of grid being added as a macro. |
| `-name` | Defines a name to use when referring to this grid definition. |
| `-voltage_domain` | Defines the name of the voltage domain for this grid. (Default: Last domain created) |
| `-starts_with` | Specifies whether the first strap placed will be POWER or GROUND (Default: GROUND) |
| `-grid_over_boundary` | Place the power grid over the entire macro. |
| `-grid_over_pg_pins` | Place the power grid over the power ground pins of the macro. (Default) |
| `-instances` | For a macro, defines a set of valid instances. Macros with a matching instance name will use this grid specification. |
| `-cells` | For a macro, defines a set of valid cells. Macros which are instances of one of these cells will use this grid specification. |
| `-default` | For a macro, specifies this is a default grid that can be overwritten. |
| `-orient` | For a macro, defines a set of valid orientations. LEF orientations (N, FN, S, FS, E, FE, W and FW) can be used as well as standard geometry orientations (R0, R90, R180, R270, MX, MY, MXR90 and MYR90). Macros with one of the valid orientations will use this grid specification. |
| `-halo` | Specifies the default minimum separation of selected macros from other cells in the design. This is only used if the macro does not define halo values in the LEF description. If 1 value is specified it will be used on all 4 sides, if two values are specified, the first will be applied to left/right sides and the second will be applied to top/bottom sides, if 4 values are specified, then they are applied to left, bottom, right and top sides respectively. (Default: 0) |
| `-obstructions` | Specify the layers to add routing blockages, in order to avoid DRC violations |

##### Examples

```
define_pdn_grid -macro -name ram          -orient {R0 R180 MX MY} -grid_over_pg_pins  -starts_with POWER -pin_direction vertical
define_pdn_grid -macro -name rotated_rams -orient {E FE W FW}     -grid_over_boundary -starts_with POWER -pin_direction horizontal
```

#### Define a grid for an existing routing

```
define_pdn_grid [-name <name>] \
                -existing \
                [-obstructions <list_of_layers>]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | Defines a name to use when referring to this grid definition. Defaults to `existing_grid` |
| `-obstructions` | Specify the layers to add routing blockages, in order to avoid DRC violations |

##### Examples

```
define_pdn_grid -name main_grid -existing
```

#### Power switch insertion

```
define_pdn_grid [-name <name>] \
                [-switch_cell <power_switch_cell_name> ] \
                [-power_control <power_constrol_signal_name>] \
                [-power_control_network (STAR|DAISY)]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-switch_cell` | Defines the name of the coarse grain power switch cell to be used for this grid. |
| `-power_control` | Defines the name of the power control signal used to control the switching of the inserted power switches. |
| `-power_control_network` | Defines the structure of the power control signal network. Choose from STAR, or DAISY |


The `-switch_cell` argument is used to specify the name of a coarse-grain power switch cell that is to be inserted whereever the stdcell rail connects to the rest of the power grid. The mesh layers are associated with the unswitched power net of the voltage domain, whereas the stdcell rail is associated with the switched power net of the voltage domain. The placement of a power switch cell connects the unswitched power mesh to the switched power rail through a power switch defined by the `define_power_switch_cell` command.

The `-power_control` argument specifies the name of the power control signal that must be connected to the inserted power control cells.

The `-power_control_network` argument specifies how the power control signal is to be connected to the power switches. If STAR is specified, then the network is wired as a high-fanout net with the power control signal driving the power control pin on every power switch. If DAISY is specified then the power switches are connected in a daisy-chain configuration - note, this requires that the power swich defined by the `define_power_switch_cell`  command defines an acknowledge pin for the switch.

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
                [-number_of_straps count] \
                [-nets list_of_nets]
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
| `-starts_with` | Specifies whether the first strap placed will be POWER or GROUND (Default: grid setting) |
| `-followpins` | Indicates that the stripe forms part of the stdcell rails, pitch and spacing are dictated by the stdcell rows, the `-width` is not needed if it can be determined from the cells |
| `-extend_to_boundary` | Extend the stripes to the boundary of the grid |
| `-snap_to_grid` | Snap the stripes to the defined routing grid |
| `-number_of_straps` | Number of power/ground pairs to add |
| `-nets` | Limit straps to just this list of nets |

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
             [-connect_to_pad_layers layers] \
             [-starts_with (POWER|GROUND)] \
             [-nets list_of_nets]
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
| `-starts_with` | Specifies whether the first strap placed will be POWER or GROUND (Default: grid setting) |
| `-nets` | Limit straps to just this list of nets |

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
                [-fixed_vias list_of_fixed_vias] \
                [-dont_use_vias list_of_vias] \
                [-max_rows rows] \
                [-max_columns columns] \
                [-ongrid ongrid_layers] \
                [-split_cuts split_cuts_mapping]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-grid` | Specifies the name of the grid definition to which this connection will be added. (Default: Last grid created by `define_pdn_grid`) |
| `-layers` | Layers to be connected where there are overlapping power or overlapping ground nets |
| `-cut_pitch` | When the two layers are parallel e.g. overlapping stdcell rails, specify the distance between via cuts |
| `-fixed_vias` | List of fixed vias to be used to form the via stack |
| `-dont_use_vias` | List or pattern of vias to not use to form the via stack |
| `-max_rows` | Maximum number of rows when adding arrays of vias |
| `-max_columns` | Maximum number of columns when adding arrays of vias |
| `-ongrid` | List of intermediate layers in a via stack to snap onto a routing grid |
| `-split_cuts` | Specifies layers to use split cuts on with an associated pitch, for example `{metal3 0.380 metal5 0.500}`. |

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
       [-report_only] \
       [-failed_via_report file]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-skip_trim` | Skip the metal trim step, which attempts to remove metal stubs |
| `-dont_add_pins` | Prevent the creation of block pins during |
| `-reset` | Reset the grid and domain specifications |
| `-ripup` | Ripup the existing power grid, as specified by the voltage domains |
| `-report_only` | Print the current specifications |
| `-failed_via_report` | Generate a report file which can be viewed in the DRC viewer for all the failed vias (ie. those that did not get built or were removed). |

### Repairing power grid vias after detailed routing

To remove vias which generate DRC violations after detailed placement and routing use `repair_pdn_vias`.

```
repair_pdn_vias [-all] \
                [-net net_name]
```

##### Options

| Name | Description |
| ----- | ----- |
| `-all` | Repair vias on all supply nets |
| `-net` | Repair only vias on the specified net |

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
