# Chip-level Connections

The chip-level connections module in OpenROAD (`pad`) is based on the
open-source tool ICeWall. In this utility, either place an IO ring around the
boundary of the chip and connect with either wirebond pads or a bump array.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Placing Terminals

In the case where the bond pads are integrated into the padcell, the IO terminals need to be placed.
To place a terminals on the padring

```tcl
place_io_terminals
    -allow_non_top_layer
    inst_pins
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-allow_non_top_layer` | Allow the terminal to be placed below the top layer. |
| `inst_pins` | Instance pins to place the terminals on. |

#### Examples
```
place_io_terminals u_*/PAD
place_io_terminals u_*/VDD
```

### Defining a Bump Array

To define a bump array.

```tcl
make_io_bump_array 
    -bump master
    -origin {x y}
    -rows rows
    -columns columns
    -pitch {x y}
    [-prefix prefix]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-bump` | Name of the bump master. |
| `-origin` | Origin of the array. |
| `-rows` | Number of rows to create. |
| `-columns` | Number of columns to create. |
| `-pitch` | Pitch of the array. |
| `-prefix` | Name prefix for the bump array. The default value is `BUMP_`. |
Example usage:

```tcl
make_io_bump_array -bump BUMP -origin "200 200" -rows 14 -columns 14 -pitch "200 200"
```

### Removing Entire Bump Array

To remove a bump array.

```tcl
remove_io_bump_array -bump master
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-bump` | Name of the bump master. |

Example usage:

```tcl
remove_io_bump_array -bump BUMP
```

### Removing a Single Bump Instance

To remove a single bump instance.

```tcl
remove_io_bump instance_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `instance_name` | Name of the bump. |

### Assigning a Net to a Bump

To assign a net to a bump.

```tcl
assign_io_bump 
    -net net
    [-terminal iterm]
    [-dont_route]
    instance
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | Net to connect to. |
| `-terminal` | Instance terminal to route to. |
| `-dont_route` | Flag to indicate that this bump should not be routed, only perform assignment. |
| `instance` | Name of the bump. |

Example usage:

```tcl
assign_io_bump -net p_ddr_addr_9_o BUMP_6_0
assign_io_bump -net p_ddr_addr_8_o BUMP_6_2
assign_io_bump -net DVSS BUMP_6_4
assign_io_bump -net DVDD BUMP_7_3
assign_io_bump -net DVDD -terminal u_dvdd/DVDD BUMP_8_3
assign_io_bump -net p_ddr_addr_7_o BUMP_7_1
assign_io_bump -net p_ddr_addr_6_o BUMP_7_0
```

### Define IO Rows

Define an IO site for the pads to be placed into.

```tcl
make_io_sites 
    -horizontal_site site
    -vertical_site site
    -corner_site site
    -offset offset
    [-rotation_horizontal rotation]
    [-rotation_vertical rotation]
    [-rotation_corner rotation]
    [-ring_index index]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-horizontal_site` | Name of the site for the horizontal pads (east and west). |
| `-vertical_site` | Name of the site for the vertical pads (north and south). |
| `-corner_site` | Name of the site for the corner cells. |
| `-offset` | Offset from the die edge to place the rows. |
| `-rotation_horizontal` | Rotation to apply to the horizontal sites to ensure pads are placed correctly. The default value is `R0`. |
| `-rotation_vertical` | Rotation to apply to the vertical sites to ensure pads are placed correctly. The default value is `R0`. |
| `-rotation_corner` | Rotation to apply to the corner sites to ensure pads are placed correctly. The default value is `R0`. |
| `-ring_index` | Used to specify the index of the ring in case of multiple rings. |

Example usage:

```tcl
make_io_sites -horizontal_site IOSITE_H -vertical_site IOSITE_V -corner_site IOSITE_C -offset 35
make_io_sites -horizontal_site IOSITE_H -vertical_site IOSITE_V -corner_site IOSITE_C -offset 35 -rotation_horizontal R180
```

### Remove IO Rows

When the padring is complete, the following command can remove the IO rows to avoid causing confusion with the other tools.

```tcl
remove_io_rows
```

### Placing Corners

To place the corner cells

```tcl
place_corners 
    master
    [-ring_index index]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `master` | Name of the master for the corners. |
| `-ring_index` | Used to specify the index of the ring in case of multiple rings. |

Example usage:

```tcl
place_corners sky130_fd_io__corner_bus_overlay
```

### Placing Pads

To place a pad into the pad ring.

```tcl
place_pad 
    -row row_name
    -location offset
    -mirror
    [-master master]
    name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-row` | Name of the row to place the pad into, examples include: `IO_NORTH`, `IO_SOUTH`, `IO_WEST`, `IO_EAST`, `IO_NORTH_0`, `IO_NORTH_1`. |
| `-location` | Offset from the bottom left chip edge to place the pad at. |
| `-mirror` | Specifies if the pad should be mirrored. |
| `-master` | Name of the instance master if the instance needs to be created. |
| `name` | Name of the instance. |

Example usage:

```tcl
place_pad -row IO_SOUTH -location 280.0 {u_clk.u_in}
place_pad -row IO_SOUTH -location 360.0 -mirror {u_reset.u_in}
place_pad -master sky130_fd_io__top_ground_hvc_wpad -row IO_SOUTH -location 439.5 {u_vzz_0}
place_pad -master sky130_fd_io__top_power_hvc_wpad -row IO_SOUTH -location 517.5 {u_v18_0}
```

### Placing IO Filler Cells

To place the IO filler cells.

```tcl
place_io_fill 
    -row row_name
    [-permit_overlaps masters]
    masters
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-row` | Name of the row to place the pad into, examples include: `IO_NORTH`, `IO_SOUTH`, `IO_WEST`, `IO_EAST`, `IO_NORTH_0`, `IO_NORTH_1`. |
| `-permit_overlaps` | Names of the masters for the IO filler cells that allow for overlapping. |
| `masters` | Names of the masters for the IO filler cells. |

Example usage: 

```tcl
place_io_fill -row IO_NORTH s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
place_io_fill -row IO_SOUTH s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
place_io_fill -row IO_WEST s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
place_io_fill -row IO_EAST s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
```

### Connecting Ring Signals

Once the ring is complete, use the following command to connect the ring signals.

```tcl
connect_by_abutment
```

### Placing Wirebond Pads

To place the wirebond pads over the IO cells.

```tcl
place_bondpad 
    -bond master
    [-offset {x y}]
    [-rotation rotation]
    io_instances
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-bond` | Name of the bondpad master. |
| `-offset` | Offset to place the bondpad at with respect to the io instance. |
| `-rotation` | Rotation of the bondpad. |
| `io_instances` | Names of the instances to add bond pads to. |

Example usage:

```tcl
place_bondpad -bond PAD IO_*
```

### Creating False IO Sites

If the library does not contain sites for the IO cells, the following command can be used to add them.
This should not be used unless the sites are not in the library.

```tcl
make_fake_io_site 
    -name name
    -width width
    -height height
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | Name of the site. |
| `-width` | Width of the site (in microns). |
| `-height` | Height of the site (in microns). |

Example usage:

```tcl
make_fake_io_site -name IO_HSITE -width 1 -height 204
make_fake_io_site -name IO_VSITE -width 1 -height 200
make_fake_io_site -name IO_CSITE -width 200 -height 204
```

### Redistribution Layer Routing

To route the RDL for the bump arrays.

```tcl
rdl_route 
    -layer layer
    [-bump_via access_via]
    [-pad_via access_via]
    [-width width]
    [-spacing spacing]
    [-turn_penalty penalty]
    [-allow45]
    nets
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layer` | Layer to route on. |
| `-bump_via` | Via to use to to connect the bump to the routing layer. |
| `-pad_via` | Via to use to to connect the pad cell to the routing layer. |
| `-width` | Width of the routing. Defaults to minimum width for each respective layer. |
| `-spacing` | Spacing of the routing. Defaults to minimum spacing for each respective layer. |
| `-turn_penalty` | Scaling factor to apply to discurage turning to allow for straighter routes. The default value is `2.0`, and the allowed values are floats. |
| `-allow45` | Specifies that 45 degree routing is permitted. |
| `nets` | Nets to route. |

### Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/ICeWall.cpp) or the [swig file](./src/pad.i).

| Command Name | Description |
| ----- | ----- |
| `find_site` | Find site given site name. |
| `find_master` | Find master given master name. |
| `find_instance` | Find instance given instance name. |
| `find_net` | Find net given net name. |
| `assert_required` | Assert argument that is required for `cmd` |
| `connect_iterm` | Connect instance terminals. Required inputs are: `inst_name`, `iterm_name`, `net_name`. |
| `convert_tcl` | These functions read from $ICeWall::library parameters to generate a standalone Tcl script. |

## Example Scripts

Example scripts for running ICeWall functions can be found in `./test`.

```tcl
./test/assign_bumps.tcl
./test/bump_array_make.tcl
./test/bump_array_remove.tcl
./test/bump_array_remove_single.tcl
./test/connect_by_abutment.tcl
./test/make_io_sites.tcl
./test/place_bondpad.tcl
./test/place_bondpad_stagger.tcl
./test/place_pad.tcl
./test/rdl_route.tcl
./test/rdl_route_45.tcl
./test/rdl_route_assignments.tcl
```

## Regression Tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+pad)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
