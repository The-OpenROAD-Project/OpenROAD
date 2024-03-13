# Power Distribution Network Generator

The power distribution network (PDN) generator module in OpenROAD (`pdn`) 
is based on the PDNGEN tool. 
This utility aims to simplify the process of adding a power grid into a
floorplan. The aim is to specify a small set of power grid policies to be
applied to the design, such as layers to use, stripe width and spacing,
then have the utility generate the actual metal straps. Grid policies can
be defined over the stdcell area, and over areas occupied by macros.

```{seealso}
To work with UPF files, refer to [Read UPF Utility](../upf/README.md).
```

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Build Power Grid

Build a power grid in accordance with the information specified.

```tcl
pdngen 
    [-skip_trim]
    [-dont_add_pins]
    [-reset]
    [-ripup]
    [-report_only]
    [-failed_via_report file]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-skip_trim` | Skip the metal trim step, which attempts to remove metal stubs. |
| `-dont_add_pins` | Prevent the creation of block pins. |
| `-reset` | Reset the grid and domain specifications. |
| `-ripup` | Ripup the existing power grid, as specified by the voltage domains. |
| `-report_only` | Print the current specifications. |
| `-failed_via_report` | Generate a report file that can be viewed in the DRC viewer for all the failed vias (i.e., those that did not get built or were removed). |

### Define Voltage Domains

Defines a named voltage domain with the names of the power and ground nets for a region.

This region must already exist in the floorplan before referencing it with the `set_voltage_domain` command. If the `-region` argument is not supplied, then the region is the entire core area of the design.

```tcl
set_voltage_domain 
    -name domain_name
    -power power_net_name 
    -ground ground_net_name
    [-region region_name]
    [-secondary_power secondary_power_net] 
    [-switched_power switched_power_net]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | Defines the name of the voltage domain. The default is "Core" or region name if provided. |
| `-power` | Specifies the name of the power net for this voltage domain. |
| `-ground` | Specifies the name of the ground net for this voltage domain. |
| `-region` | Specifies a region of the design occupied by this voltage domain. |
| `-secondary_power` | Specifies the name of the secondary power net for this voltage domain. |
| `-switched_power` | Specifies the name of the switched power net for switched power domains. |

Example usage:

```tcl
set_voltage_domain -power VDD -ground VSS
set_voltage_domain -name TEMP_ANALOG -region TEMP_ANALOG -power VIN -ground VSS
set_voltage_domain -region test_domain -power VDD -ground VSS -secondary_power VREG
```

### Define Power Grid (General)

Define the rules to describe a power grid pattern to be placed in the design.

```{warning}
`define_pdn_grid` is overloaded with two different signatures. Take note of the arguments when using this function!
```

```tcl
define_pdn_grid 
    [-name name] 
    [-voltage_domain list_of_domain_names] 
    [-pins list_of_pin_layers] 
    [-starts_with POWER|GROUND] 
    [-starts_with POWER|GROUND] 
    [-obstructions list_of_layers]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | The name to use when referring to this grid definition. |
| `-voltage_domain` | This grid's voltage domain name. Defaults to the last domain created. |
| `-pins` | List of layers where the power straps will be promoted to block pins. |
| `-starts_with` | Use `POWER` or `GROUND`  for the first placed strap. Defaults to `GROUND`. |
| `-obstructions` | Layers to add routing blockages to avoid DRC violations. |

Example usage:

```tcl
define_pdn_grid -name main_grid -pins {metal7} -voltage_domain {CORE TEMP_ANALOG}
```

### Define Power Grid (Macros)

```tcl
define_pdn_grid 
    -macro
    [-name name]
    [-grid_over_pg_pins|-grid_over_boundary]
    [-voltage_domain list_of_domain_names]
    [-orient list_of_valid_orientations]
    [-instances list_of_instances]
    [-cells list_of_cells]
    [-default]
    [-halo list_of_halo_values]
    [-pins list_of_pin_layers]
    [-starts_with POWER|GROUND]
    [-obstructions list_of_layers]
    [-power_switch_cell name]
    [-power_control signal_name]
    [-power_control_network STAR|DAISY]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-macro` | The type of grid added as a macro. |
| `-name` | The name to use when referring to this grid definition. |
| `-grid_over_pg_pins`, `-grid_over_boundary` | Place the power grid over the power ground pins of the macro (default) or place the power grid over the entire macro.  |
| `-voltage_domain` | Grid's voltage domain name. Defaults to the last domain created. |
| `-orient` | For a macro, defines a set of valid orientations. LEF orientations (N, FN, S, FS, E, FE, W and FW) can be used as well as standard geometry orientations (R0, R90, R180, R270, MX, MY, MXR90 and MYR90). Macros with one of the valid orientations will use this grid specification. |
| `-instances` | For a macro, defines a set of valid instances. Macros with a matching instance name will use this grid specification. |
| `-cells` | For a macro, defines a set of valid cells. Macros, which are instances of one of these cells, will use this grid specification. |
| `-default` | For a macro, specifies this is a default grid that can be overwritten. |
| `-halo` | Specifies the design's default minimum separation of selected macros from other cells. This is only used if the macro does not define halo values in the LEF description. If one value is specified, it will be used on all four sides; if two values are specified, the first will be applied to left/right sides, and the second will be applied to top/bottom sides; if four values are specified, then they are applied to left, bottom, right and top sides respectively (Default: 0). |
| `-pins` | Defines a list of layers where the power straps will be promoted to block pins. |
| `-starts_with` | Use `POWER` or `GROUND`  for the first placed strap. Defaults to `GROUND`.|
| `-obstructions` | Specify the layers to add routing blockages in order to avoid DRC violations. |
| `-power_switch_cell` | Defines the name of the coarse grain power switch cell to be used wherever the stdcell rail connects to the rest of the power grid. The mesh layers are associated with the unswitched power net of the voltage domain, whereas the stdcell rail is associated with the switched power net of the voltage domain. The placement of a power switch cell connects the unswitched power mesh to the switched power rail through a power switch defined by the `define_power_switch_cell` command. |
| `-power_control` | Defines the name of the power control signal used to control the switching of the inserted power switches. |
| `-power_control_network` | Defines the structure of the power control signal network. Choose from STAR or DAISY. If STAR is specified, then the network is wired as a high-fanout net with the power control signal driving the power control pin on every power switch. If DAISY is specified, then the power switches are connected in a daisy-chain configuration - note, this requires that the power switch defined by the `define_power_switch_cell`  command defines an acknowledge pin for the switch. |

Example usage:

```tcl
define_pdn_grid -macro -name ram          -orient {R0 R180 MX MY} -grid_over_pg_pins  -starts_with POWER -pin_direction vertical
define_pdn_grid -macro -name rotated_rams -orient {E FE W FW}     -grid_over_boundary -starts_with POWER -pin_direction horizontal
```

### Define Power Grid for an Existing Routing Solution

```tcl
define_pdn_grid 
    -existing
    [-name name]
    [-obstructions list_of_layers]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-existing` | Enable use of existing routing solution. |
| `-name` | The name to use when referring to this grid definition. Defaults to `existing_grid`. |
| `-obstructions` | The layers to add routing blockages in order to avoid DRC violations. |

Example usage:

```tcl
define_pdn_grid -name main_grid -existing
```

### Define Power Switch Cell

Define a power switch cell that will be inserted into a power grid.

```tcl
define_power_switch_cell 
    -name name 
    -control control_pin
    -power_switchable power_switchable_pin
    -power unswitched_power_pin
    -ground ground_pin 
    [-acknowledge acknowledge_pin_name]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | The name of the power switch cell. |
| `-control` | The name of the power control port of the power switch cell. |
| `-switched_power` | The pin's name that outputs the switched power net. |
| `-power` | The pin's name that connects to the unswitched power net. |
| `-ground` | The pin's name that connects to the ground net. |
| `-acknowledge` | The name of the output control signal of the power control switch if it has one. |

Example usage:

```tcl
define_power_switch_cell -name POWER_SWITCH -control SLEEP -switched_power VDD -power VDDG -ground VSS
```

### Add Stripes

Defines a pattern of power and ground stripes in a single layer to be added to a power grid.

```tcl
add_pdn_stripe 
    -layer layer_name
    [-grid grid_name]
    [-width width_value]
    [-followpins]
    [-extend_to_core_ring]
    [-pitch pitch_value]
    [-spacing spacing_value]
    [-offset offset_value]
    [-starts_with POWER|GROUND]
    [-extend_to_boundary]
    [-snap_to_grid]
    [-number_of_straps count]
    [-nets list_of_nets]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layer` | The layer name for these stripes. |
| `-grid` | The grid to which this stripe definition will be added. (Default: Last grid defined by `define_pdn_grid`). |
| `-width` | Value for the width of the stripe. |
| `-followpins` | Indicates that the stripe forms part of the stdcell rails, pitch and spacing are dictated by the stdcell rows, and the `-width` is not needed if it can be determined from the cells. |
| `-extend_to_core_ring` | Extend the stripes to the core PG ring. |
| `-pitch` | Value for the distance between each power/ground pair. |
| `-spacing` | Optional specification of the spacing between power/ground pairs within a single pitch defaults to `pitch / 2`. |
| `-offset` | Value for the offset of the stripe from the lower left corner of the design core area. |
| `-starts_with` | Use `POWER` or `GROUND`  for the first placed strap. Defaults to `GROUND`. |
| `-extend_to_boundary` | Extend the stripes to the boundary of the grid. |
| `-snap_to_grid` | Snap the stripes to the defined routing grid. |
| `-number_of_straps` | Number of power/ground pairs to add. |
| `-nets` | Limit straps to just this list of nets. |

Example usage:

```tcl
add_pdn_stripe -grid main_grid -layer metal1 -followpins
add_pdn_stripe -grid main_grid -layer metal2 -width 0.17 -followpins
add_pdn_stripe -grid main_grid -layer metal4 -width 0.48 -pitch 56.0 -offset 2 -starts_with GROUND
```

### Add Rings

The `add_pdn_ring` command defines power/ground rings around a grid region. The ring structure is built using two layers that are orthogonal to each other. A power/ground pair will be added above and below the grid using the horizontal layer, with another power/ground pair to the left and right using the vertical layer. These four pairs of power/ground stripes form a ring around the specified grid. Power straps on these layers that are inside the enclosed region are extended to connect to the ring.

```tcl
add_pdn_ring 
    -layers layer_name
    -widths width_value|list_of_2_values
    -spacings spacing_value|list_of_2_values
    [-grid grid_name]
    [-core_offsets offset_value]
    [-pad_offsets offset_value]
    [-add_connect]
    [-extend_to_boundary]
    [-connect_to_pads]
    [-connect_to_pad_layers layers]
    [-starts_with POWER|GROUND]
    [-nets list_of_nets]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layers` | Specifies the name of the layer for these stripes. |
| `-widths` | Value for the width of the stdcell rail. |
| `-spacings` | Optional specification of the spacing between power/ground pairs within a single pitch. (Default: pitch / 2). |
| `-grid` | Specifies the name of the grid to which this ring defintion will be added. (Default: Last grid created by `define_pdn_grid`). |
| `-core_offsets` | Value for the offset of the ring from the grid region. |
| `-pad_offsets` | When defining a power grid for the top level of an SoC, can be used to define the offset of ring from the pad cells. |
| `-add_connect` | Automatically add a connection between the two layers. |
| `-extend_to_boundary` | Extend the rings to the grid boundary. |
| `-connect_to_pads` | The core side of the pad pins will be connected to the ring. |
| `-connect_to_pad_layers` | Restrict the pad pins layers to this list. |
| `-starts_with` | Use `POWER` or `GROUND`  for the first placed strap. Defaults to `GROUND`. |
| `-nets` | Limit straps to just this list of nets. |

Example usage: 

```tcl
add_pdn_ring -grid main_grid -layer {metal6 metal7} -widths 5.0 -spacings  3.0 -core_offset 5
```

### Add Connections

The `add_pdn_connect` command is used to define which layers in the power grid are to be connected together. During power grid generation, vias will be added for overlapping power nets and overlapping ground nets. The use of fixed vias from the technology file can be specified or else via stacks will be constructed using VIARULEs. If VIARULEs are not available in the technology, then fixed vias must be used.

```tcl
add_pdn_connect 
    -layers list_of_two_layers
    [-grid grid_name]
    [-cut_pitch pitch_value]
    [-fixed_vias list_of_fixed_vias]
    [-dont_use_vias list_of_vias]
    [-max_rows rows]
    [-max_columns columns]
    [-ongrid ongrid_layers]
    [-split_cuts split_cuts_mapping]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layers` | Layers to be connected where there are overlapping power or overlapping ground nets. |
| `-grid` | Specifies the name of the grid definition to which this connection will be added (Default: Last grid created by `define_pdn_grid`). |
| `-cut_pitch` | When the two layers are parallel, e.g., overlapping stdcell rails, specify the distance between via cuts. |
| `-fixed_vias` | List of fixed vias to be used to form the via stack. |
| `-dont_use_vias` | List or pattern of vias to not use to form the via stack. |
| `-max_rows` | Maximum number of rows when adding arrays of vias. |
| `-max_columns` | Maximum number of columns when adding arrays of vias. |
| `-ongrid` | List of intermediate layers in a via stack to snap onto a routing grid. |
| `-split_cuts` | Specifies layers to use split cuts on with an associated pitch, for example `{metal3 0.380 metal5 0.500}`. |

Example usage:

```tcl
add_pdn_connect -grid main_grid -layers {metal1 metal2} -cut_pitch 0.16
add_pdn_connect -grid main_grid -layers {metal2 metal4}
add_pdn_connect -grid main_grid -layers {metal4 metal7}

add_pdn_connect -grid ram -layers {metal4 metal5}
add_pdn_connect -grid ram -layers {metal5 metal6}
add_pdn_connect -grid ram -layers {metal6 metal7}

add_pdn_connect -grid rotated_rams -layers {metal4 metal6}
add_pdn_connect -grid rotated_rams -layers {metal6 metal7}
```

### Repairing Power Grid vias after Detailed Routing

To remove vias which generate DRC violations after detailed placement and routing use `repair_pdn_vias`.

```tcl
repair_pdn_vias 
    [-all]
    [-net net_name]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-all` | Repair vias on all supply nets. |
| `-net` | Repair only vias on the specified net. |

### Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](../src/PdnGen.cc) or the [swig file](PdnGen.i).

| Command Name | Description |
| ----- | ----- |
| `name_cmp` | Compare 2 input strings `obj1` and `obj2` if they are equal. |
| `check_design_state` | Check if the design is loaded. |
| `get_layer` | Get the layer reference of the layer name. |
| `get_voltage_domains` | Gets a Tcl list of power domains in design. |
| `match_orientation` | Checks if a given orientation `orient` is within a list of orientations `orients`. |
| `get_insts` | Get the Tcl list of instances. |
| `get_masters` | Get the Tcl list of masters. |
| `get_one_to_two` | If a Tcl list has one element `{x}`, Tcl list `{x x}` is returned. If a Tcl list of two elements `{y y}`, list as is returned. Otherwise, for any other list lengths, an error is triggered. |
| `get_one_to_four` | Similar logic for the above function, except the logic only works for lists of length one, two, and four, respectively. All other list lengths trigger errors. |
| `get_obstructions` | Get the Tcl list of layers. |
| `get_starts_with` | If value starts with `POWER`, return 1; else if value starts with `GROUND` return 0; else return error. |
| `get_mterm` | Find master terminal. |
| `get_orientations` | Get the list of valid orientations. | 

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

### Sroute

The `add_sroute_connect` command is employed for connecting pins located
outside of a specific power domain to the power ring, especially in cases where
multiple power domains are present. During `sroute`, multi-cut vias will be added
for new connections. The use of fixed vias from the technology file should be
specified for the connection using the `add_sroute_connect` command. The use
of max_rows and max_columns defines the row and column limit for the via stack.

```
add_sroute_connect
    -layers list_of_2_layers
    -cut_pitch pitch_value
    [-net net]
    [-outerNet outerNet]
    [-fixed_vias list_of_vias]
    [-max_rows rows]
    [-max_columns columns]
    [-metalwidths metalwidths]
    [-metalspaces metalspaces]
    [-ongrid ongrid_layers]
    [-insts inst]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | The inner net where the power ring exists. |
| `-outerNet` | The outer net where instances/pins that need to get connected exist. |
| `-layers` |  The metal layers for vertical stripes within inner power ring. |
| `-cut_pitch` | Distance between via cuts when the two layers are parallel, e.g., overlapping stdcell rails. (Default:200 200) |
| `-fixed_vias` | List of fixed vias to be used to form the via stack. |
| `-max_rows` | Maximum number of rows when adding arrays of vias. (Default:10) |
| `-max_columns` | Maximum number of columns when adding arrays of vias. (Default:10) |
| `-metalwidths` | Width for each metal layer. |
| `-metalspaces` | Spacing of each metal layer. |
| `-ongrid` | List of intermediate layers in a via stack to snap onto a routing grid. |
| `-insts` | List of all the instances that contain the pin that needs to get connected with power ring. (Default:nothing) |

#### Examples

```
add_sroute_connect  -net "VIN" -outerNet "VDD" -layers {met1 met4} -cut_pitch {200 200} -fixed_vias {M3M4_PR_M} -metalwidths {1000 1000} -metalspaces {800} -ongrid {met3 met4} -insts "temp_analog_1.a_header_0"

```
## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

Currently the following assumptions are made:

1. The design is rectangular
1. The input floorplan includes the stdcell rows, placement of all macro blocks and IO pins.
1. The stdcells rows will be cut around macro placements

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+pdn) about this tool.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
