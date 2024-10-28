# Power Distribution Network Generator

The power distribution network (PDN) generator module in OpenROAD (`pdn`) 
is based on the PDNGEN tool. 
This utility aims to simplify the process of adding a power grid into a
floorplan. The aim is to specify a small set of power grid policies to be
applied to the design, such as layers to use, stripe width and spacing,
then have the utility generate the actual metal straps. Grid policies can
be defined over the stdcell area, and over areas occupied by macros.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Build Power Grid

Build a power grid in accordance with the information specified.

```tcl
pdngen
    [-dont_add_pins]
    [-failed_via_report file]
    [-report_only]
    [-reset]
    [-ripup]
    [-skip_trim]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `[-dont_add_pins]` | Prevent the creation of block pins. |
| `[-failed_via_report]` | Generate a report file which can be viewed in the DRC viewer for all the failed vias (ie. those that did not get built or were removed). |
| `[-report_only]` | Print the current specifications. |
| `[-reset]` | Reset the grid and domain specifications. |
| `[-ripup]` | Ripup the existing power grid, as specified by the voltage domains. |
| `[-skip_trim]` | Skip the metal trim step, which attempts to remove metal stubs. |

### Define Voltage Domain

Defines a named voltage domain with the names of the power and ground nets for a region.

This region must already exist in the floorplan before referencing it with the `set_voltage_domain` command. If the `-region` argument is not supplied then region is the entire core area of the design.

Example usage:

```
set_voltage_domain -power VDD -ground VSS
set_voltage_domain -name TEMP_ANALOG -region TEMP_ANALOG -power VIN -ground VSS
set_voltage_domain -region test_domain -power VDD -ground VSS -secondary_power VREG
```

```tcl
set_voltage_domain
    -ground ground_net_name
    -name domain_name
    -power power_net_name
    [-region region_name]
    [-secondary_power secondary_power_net]
    [-switched_power switched_power_net]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-ground` | Specifies the name of the ground net for this voltage domain. |
| `-name` | Defines the name of the voltage domain. The default is "Core" or region name if provided. |
| `-power` | Specifies the name of the power net for this voltage domain. |
| `[-region]` | Specifies a region of the design occupied by this voltage domain. |
| `[-secondary_power]` | Specifies the name of the secondary power net for this voltage domain. |
| `[-switched_power]` | Specifies the name of the switched power net for switched power domains. |

### Define Power Grids

```{warning}
`define_pdn_grid` is overloaded with two different signatures. Take note of the arguments when using this function!
```

- Method 1: General Usage
Define the rules to describe a power grid pattern to be placed in the design.

Example usage:

```
define_pdn_grid -name main_grid -pins {metal7} -voltage_domain {CORE TEMP_ANALOG}
```

- Method 2: Macros
Define the rules for one or more macros.

Example usage:

```
define_pdn_grid -macro -name ram          -orient {R0 R180 MX MY} -grid_over_pg_pins  -starts_with POWER
define_pdn_grid -macro -name rotated_rams -orient {E FE W FW}     -grid_over_boundary -starts_with POWER
```

- Method 3: Modify existing power domain
Modify pre-existing power domain.

Example usage:

```
define_pdn_grid -name main_grid -existing
```

```tcl
define_pdn_grid
    [-cells list_of_cells]
    [-default]
    [-existing]
    [-grid_over_pg_pins|-grid_over_boundary]
    [-halo list_of_halo_values]
    [-instances list_of_instances]
    [-macro]
    [-name name]
    [-obstructions list_of_layers]
    [-orient list_of_valid_orientations]
    [-pins list_of_pin_layers]
    [-power_control signal_name]
    [-power_control_network STAR|DAISY]
    [-power_switch_cell name]
    [-starts_with POWER|GROUND]
    [-voltage_domains list_of_domain_names]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `[-cells]` | For a macro, defines a set of valid cells. Macros which are instances of one of these cells will use this grid specification. |
| `[-default]` | For a macro, specifies this is a default grid that can be overwritten. |
| `[-existing]` | Flag to enable defining for existing routing solution. |
| `[-grid_over_pg_pins]`, `[-grid_over_boundary]` | Place the power grid over the power ground pins of the macro. (Default True), or Place the power grid over the entire macro. |
| `[-halo]` | Specifies the default minimum separation of selected macros from other cells in the design. This is only used if the macro does not define halo values in the LEF description. If 1 value is specified it will be used on all 4 sides, if two values are specified, the first will be applied to left/right sides and the second will be applied to top/bottom sides, if 4 values are specified, then they are applied to left, bottom, right and top sides respectively (Default: 0). |
| `[-instances]` | For a macro, defines a set of valid instances. Macros with a matching instance name will use this grid specification. |
| `[-macro]` | Defines the type of grid being added as a macro. |
| `[-name]` | Defines a name to use when referring to this grid definition. |
| `[-obstructions]` | Specify the layers to add routing blockages, in order to avoid DRC violations. |
| `[-orient]` | For a macro, defines a set of valid orientations. LEF orientations (N, FN, S, FS, E, FE, W and FW) can be used as well as standard geometry orientations (R0, R90, R180, R270, MX, MY, MXR90 and MYR90). Macros with one of the valid orientations will use this grid specification. |
| `[-pins]` | Defines a list of layers which where the power straps will be promoted to block pins. |
| `[-power_control]` | Defines the name of the power control signal used to control the switching of the inserted power switches. |
| `[-power_control_network]` | Defines the structure of the power control signal network. Choose from STAR, or DAISY. If STAR is specified, then the network is wired as a high-fanout net with the power control signal driving the power control pin on every power switch. If DAISY is specified then the power switches are connected in a daisy-chain configuration - note, this requires that the power swich defined by the `define_power_switch_cell`  command defines an acknowledge pin for the switch. |
| `[-power_switch_cell]` | Defines the name of the coarse grain power switch cell to be used wherever the stdcell rail connects to the rest of the power grid. The mesh layers are associated with the unswitched power net of the voltage domain, whereas the stdcell rail is associated with the switched power net of the voltage domain. The placement of a power switch cell connects the unswitched power mesh to the switched power rail through a power switch defined by the `define_power_switch_cell` command. |
| `[-starts_with]` | Specifies whether the first strap placed will be POWER or GROUND (Default: GROUND). |
| `[-voltage_domains]` | Defines the name of the voltage domain for this grid (Default: Last domain created). |


### Power Switch Cell insertion

Define a power switch cell that will be inserted into a power grid 

Example usage:

```
define_power_switch_cell -name POWER_SWITCH -control SLEEP -power_switchable VDD -power VDDG -ground VSS
```

```tcl
define_power_switch_cell
    -control control_pin
    -ground ground_pin
    -name name
    -power unswitched_power_pin
    -power_switchable power_switchable_pin
    [-acknowledge acknowledge_pin_name]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-control` | The name of the power control port of the power switch cell. |
| `-ground` | Defines the name of the pin that connects to the ground net. |
| `-name` | The name of the power switch cell. |
| `-power` | Defines the name of the pin that connects to the unswitched power net. |
| `-power_switchable` | Defines the name of the pin that outputs the switched power net. |
| `[-acknowledge]` | Defines the name of the output control signal of the power control switch if it has one. |

### Add PDN Straps/Stripes

Defines a pattern of power and ground stripes in a single layer to be added to a power grid.

Example usage:

```
add_pdn_stripe -grid main_grid -layer metal1 -followpins
add_pdn_stripe -grid main_grid -layer metal2 -width 0.17 -followpins
add_pdn_stripe -grid main_grid -layer metal4 -width 0.48 -pitch 56.0 -offset 2 -starts_with GROUND
```

```tcl
add_pdn_stripe
    -layer layer_name
    [-extend_to_boundary]
    [-extend_to_core_ring]
    [-followpins]
    [-grid grid_name]
    [-nets list_of_nets]
    [-number_of_straps count]
    [-offset offset_value]
    [-pitch pitch_value]
    [-snap_to_grid]
    [-spacing spacing_value]
    [-starts_with POWER|GROUND]
    [-width width_value]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layer` | Specifies the name of the layer for these stripes. |
| `[-extend_to_boundary]` | Extend the stripes to the boundary of the grid. |
| `[-extend_to_core_ring]` | Extend the stripes to the core PG ring. |
| `[-followpins]` | Indicates that the stripe forms part of the stdcell rails, pitch and spacing are dictated by the stdcell rows, the `-width` is not needed if it can be determined from the cells. |
| `[-grid]` | Specifies the grid to which this stripe definition will be added. (Default: Last grid defined by `define_pdn_grid`). |
| `[-nets]` | Limit straps to just this list of nets. |
| `[-number_of_straps]` | Number of power/ground pairs to add. |
| `[-offset]` | Value for the offset of the stripe from the lower left corner of the design core area. |
| `[-pitch]` | Value for the distance between each power/ground pair. |
| `[-snap_to_grid]` | Snap the stripes to the defined routing grid. |
| `[-spacing]` | Optional specification of the spacing between power/ground pairs within a single pitch (Default: pitch / 2). |
| `[-starts_with]` | Specifies whether the first strap placed will be POWER or GROUND (Default: grid setting). This cannot be used with -followpins (Flip sites when initializing floorplan to change followpin power/ground order). |
| `[-width]` | Value for the width of stripe. |

### Add Sroute Connect

The `add_sroute_connect` command is employed for connecting pins located
outside of a specific power domain to the power ring, especially in cases where
multiple power domains are present. During `sroute`, multi-cut vias will be added
for new connections. The use of fixed vias from the technology file should be
specified for the connection using the `add_sroute_connect` command. The use
of max_rows and max_columns defines the row and column limit for the via stack.

Example:
```
add_sroute_connect  -net "VIN" -outerNet "VDD" -layers {met1 met4} -cut_pitch {200 200} -fixed_vias {M3M4_PR_M} -metalwidths {1000 1000} -metalspaces {800} -ongrid {met3 met4} -insts "temp_analog_1.a_header_0"
```

```tcl
add_sroute_connect
    -cut_pitch pitch_value
    -layers list_of_2_layers
    [-fixed_vias list_of_vias]
    [-insts inst]
    [-max_columns columns]
    [-max_rows rows]
    [-metalspaces metalspaces]
    [-metalwidths metalwidths]
    [-net net]
    [-ongrid ongrid_layers]
    [-outerNet outerNet]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-cut_pitch` | Distance between via cuts when the two layers are parallel, e.g., overlapping stdcell rails. (Default:200 200) |
| `-layers` |  The metal layers for vertical stripes within inner power ring. |
| `[-fixed_vias]` | List of fixed vias to be used to form the via stack. |
| `[-insts]` | List of all the instances that contain the pin that needs to get connected with power ring. (Default:nothing) |
| `[-max_columns]` | Maximum number of columns when adding arrays of vias. (Default:10) |
| `[-max_rows]` | Maximum number of rows when adding arrays of vias. (Default:10) |
| `[-metalspaces]` | Spacing of each metal layer. |
| `[-metalwidths]` | Width for each metal layer. |
| `[-net]` | The inner net where the power ring exists. |
| `[-ongrid]` | List of intermediate layers in a via stack to snap onto a routing grid. |
| `[-outerNet]` | The outer net where instances/pins that need to get connected exist. |

### Add PDN Ring

The `add_pdn_ring` command is used to define power/ground rings around a grid region.
The ring structure is built using two layers that are orthogonal to each other.
A power/ground pair will be added above and below the grid using the horizontal
layer, with another power/ground pair to the left and right using the vertical layer.
Together these 4 pairs of power/ground stripes form a ring around the specified grid.
Power straps on these layers that are inside the enclosed region are extend to 
connect to the ring.

Example usage: 

```
add_pdn_ring -grid main_grid -layer {metal6 metal7} -widths 5.0 -spacings  3.0 -core_offset 5
```

```tcl
add_pdn_ring
    -layers layer_name
    -spacings spacing_value|list_of_2_values
    -widths width_value|list_of_2_values
    [-add_connect]
    [-connect_to_pad_layers layers]
    [-connect_to_pads]
    [-core_offsets offset_value]
    [-extend_to_boundary]
    [-grid grid_name]
    [-nets list_of_nets]
    [-pad_offsets offset_value]
    [-starts_with POWER|GROUND]

```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layers` | Specifies the name of the layer for these stripes. |
| `-spacings` | Optional specification of the spacing between power/ground pairs within a single pitch. (Default: pitch / 2). |
| `-widths` | Value for the width of the stdcell rail. |
| `[-add_connect]` | Automatically add a connection between the two layers. |
| `[-connect_to_pad_layers]` | Restrict the pad pins layers to this list. |
| `[-connect_to_pads]` | The core side of the pad pins will be connected to the ring. |
| `[-core_offsets]` | Value for the offset of the ring from the grid region. |
| `[-extend_to_boundary]` | Extend the rings to the grid boundary. |
| `[-grid]` | Specifies the name of the grid to which this ring defintion will be added. (Default: Last grid created by `define_pdn_grid`). |
| `[-nets]` | Limit straps to just this list of nets. |
| `[-pad_offsets]` | When defining a power grid for the top level of an SoC, can be used to define the offset of ring from the pad cells. |
| `[-starts_with]` | Specifies whether the first strap placed will be POWER or GROUND (Default: grid setting). |

### Add PDN Connect

The `add_pdn_connect` command is used to define which layers in the power grid are to be connected together. During power grid generation, vias will be added for overlapping power nets and overlapping ground nets. The use of fixed vias from the technology file can be specified or else via stacks will be constructed using VIARULEs. If VIARULEs are not available in the technology, then fixed vias must be used.

Example usage:

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

```tcl
add_pdn_connect
    -layers list_of_two_layers
    [-cut_pitch pitch_value]
    [-dont_use_vias list_of_vias]
    [-fixed_vias list_of_fixed_vias]
    [-grid grid_name]
    [-max_columns columns]
    [-max_rows rows]
    [-ongrid ongrid_layers]
    [-split_cuts split_cuts_mapping]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layers` | Layers to be connected where there are overlapping power or overlapping ground nets. |
| `[-cut_pitch]` | When the two layers are parallel e.g. overlapping stdcell rails, specify the distance between via cuts. |
| `[-dont_use_vias]` | List or pattern of vias to not use to form the via stack. |
| `[-fixed_vias]` | List of fixed vias to be used to form the via stack. |
| `[-grid]` | Specifies the name of the grid definition to which this connection will be added (Default: Last grid created by `define_pdn_grid`). |
| `[-max_columns]` | Maximum number of columns when adding arrays of vias. |
| `[-max_rows]` | Maximum number of rows when adding arrays of vias. |
| `[-ongrid]` | List of intermediate layers in a via stack to snap onto a routing grid. |
| `[-split_cuts]` | Specifies layers to use split cuts on with an associated pitch, for example `{metal3 0.380 metal5 0.500}`. |

### Repairing power grid vias after detailed routing

To remove vias which generate DRC violations after detailed placement
and routing use `repair_pdn_vias`.

```tcl
repair_pdn_vias
    [-all]
    [-net net_name]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `[-all]` | Repair vias on all supply nets. |
| `[-net]` | Repair only vias on the specified net. |

## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](../src/PdnGen.cc) or the [swig file](PdnGen.i).

| Command Name | Description |
| ----- | ----- |
| `name_cmp` | Compare 2 input strings `obj1` and `obj2` if they are equal. |
| `check_design_state` | Check if design is loaded. |
| `get_layer` | Get the layer reference of layer name. |
| `get_voltage_domains` | Gets a Tcl list of power domains in design. |
| `match_orientation` | Checks if a given orientation `orient` is within a list of orientations `orients`. |
| `get_insts` | Get Tcl list of instances. |
| `get_masters` | Get Tcl list of masters. |
| `get_one_to_two` | If a Tcl list has one element `{x}`, Tcl list `{x x}` is returned. If a Tcl list of two elements `{y y}`, list as is returned. Otherwise, for any other list lengths, error is triggered. |
| `get_one_to_four` | Similar logic for above function, except the logic only works for lists of length one, two and four respectively. All other list lengths triggers error. |
| `get_obstructions` | Get Tcl list of layers. |
| `get_starts_with` | If value starts with `POWER`, return 1; else if value starts with `GROUND` return 0; else return error. |
| `get_mterm` | Find master terminal. |
| `get_orientations` | Get list of valid orientations. |

## Example scripts

## Defining a SoC power grid with pads

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
