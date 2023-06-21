# ICeWall

Place an IO ring around the boundary of the a chip and connect with either wirebond pads or a bump array

## Commands

### Define IO Rows

Define an IO site for the pads to be placed into
```
make_io_sites -horizontal_site site \
              -vertical_site site \
              -corner_site site \
              -offset offset \
              [-rotation_horizontal rotation] \
              [-rotation_vertical rotation] \
              [-rotation_corner rotation] \
              [-ring_index index]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-horizontal_site` | Name of the site for the horizontal pads (east and west). |
| `-vertical_site` | Name of the site for the vertical pads (north and south). |
| `-corner_site` | Name of the site for the corner cells. |
| `-offset` | Offset from the die edge to place the rows. |
| `-rotation_horizontal` | Rotation to apply to the horizontal sites to ensure pads are placed correctly (defaults to R0). |
| `-rotation_vertical` | Rotation to apply to the vertical sites to ensure pads are placed correctly (defaults to R0). |
| `-rotation_corner` | Rotation to apply to the corner sites to ensure pads are placed correctly (defaults to R0). |
| `-ring_index` | (Optional) Used to specify the index of the ring in case of multiple rings. |

#### Examples
```
make_io_sites -horizontal_site IOSITE_H -vertical_site IOSITE_V -corner_site IOSITE_C -offset 35
make_io_sites -horizontal_site IOSITE_H -vertical_site IOSITE_V -corner_site IOSITE_C -offset 35 -rotation_horizontal R180
```

### Creating False IO Sites

If the library does not contain sites for the IO cells, the following command can be used to add them.
This should not be used unless the sites are not in the library.
```
make_fake_io_site -name name \
                  -width width \
                  -height height
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | Name of the site. |
| `-width` | Width of the site in um. |
| `-height` | Height of the site in um. |

#### Examples
```
make_fake_io_site -name IO_HSITE -width 1 -height 204
make_fake_io_site -name IO_VSITE -width 1 -height 200
make_fake_io_site -name IO_CSITE -width 200 -height 204
```

### Remove IO Rows

When the padring is complete, the following command can remove the IO rows to avoid causing confusion with the other tools
```
remove_io_rows
```

### Placing Pads

To place a pad into the pad ring
```
place_pad -row row_name \
          -location offset \
          -mirror \
          [-master master] \
          name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-row` | Name of the row to place the pad into, examples include: IO_NORTH, IO_SOUTH, IO_WEST, IO_EAST, IO_NORTH_0, IO_NORTH_1 |
| `-location` | Offset from the bottom left chip edge to place the pad at. |
| `-mirror` | Specifies if the pad should be mirrored. |
| `-master` | Name of the instance master if the instance needs to be created. |
| `name` | Name of the instance |

#### Examples
```
place_pad -row IO_SOUTH -location 280.0 {u_clk.u_in}
place_pad -row IO_SOUTH -location 360.0 -mirror {u_reset.u_in}
place_pad -master sky130_fd_io__top_ground_hvc_wpad -row IO_SOUTH -location 439.5 {u_vzz_0}
place_pad -master sky130_fd_io__top_power_hvc_wpad -row IO_SOUTH -location 517.5 {u_v18_0}
```

### Placing corners

To place the corner cells
```
place_corners master \
              [-ring_index index]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-ring_index` | (Optional) Used to specify the index of the ring in case of multiple rings. |
| `master` | Name of the master for the corners. |

#### Examples
```
place_corners sky130_fd_io__corner_bus_overlay
```

### Placing IO Filler Cells

To place the IO filler cells
```
place_io_fill -row row_name \
              [-permit_overlaps masters]
              masters
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-row` | Name of the row to place the pad into, examples include: IO_NORTH, IO_SOUTH, IO_WEST, IO_EAST, IO_NORTH_0, IO_NORTH_1 |
| `-permit_overlaps` | Names of the masters for the IO filler cells that allow for overlapping. |
| `masters` | Names of the masters for the IO filler cells. |

#### Examples
```
place_io_fill -row IO_NORTH s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
place_io_fill -row IO_SOUTH s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
place_io_fill -row IO_WEST s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
place_io_fill -row IO_EAST s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
```

### Connecting Ring Signals

Once the ring is complete, use the following command to connect the ring signals
```
connect_by_abutment
```

### Placing Wirebond Pads

To place the wirebond pads over the IO cells
```
place_bondpad -bond master
              [-offset {x y}] \
              [-rotation rotation] \
              io_instances
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-bond` | Name of the bondpad master |
| `-offset` | Offset to place the bondpad at with respect to the io instance |
| `-rotation` | Rotation of the bondpad |
| `io_instances` | Names of the instances to add bond pads to. |

#### Examples
```
place_bondpad -bond PAD IO_*
```

### Defining a Bump Array

To define a bump array
```
make_io_bump_array -bump master \
                   -origin {x y} \
                   -rows rows \
                   -columns columns \
                   -pitch {x y} \
                   [-prefix prefix]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-bump` | Name of the bump master |
| `-origin` | Origin of the array |
| `-rows` | Number of rows to create |
| `-columns` | Number of columns to create |
| `-pitch` | Pitch of the array |
| `-prefix` | Name prefix for the bump array (default: BUMP_) |

#### Examples
```
make_io_bump_array -bump BUMP -origin "200 200" -rows 14 -columns 14 -pitch "200 200"
```

### Removing Entire Bump Array

To remove a bump array
```
remove_io_bump_array -bump master
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-bump` | Name of the bump master |

#### Examples
```
remove_io_bump_array -bump BUMP
```

### Removing a Single Bump Instance

To remove a single bump instance
```
remove_io_bump instance_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `instance_name` | Name of the bump |

### Assigning a Net to a Bump

To assign a net to a bump
```
assign_io_bump -net net \
               instance
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | Net to connect to |
| `instance` | Name of the bump |

#### Examples
```
assign_io_bump -net p_ddr_addr_9_o BUMP_6_0
assign_io_bump -net p_ddr_addr_8_o BUMP_6_2
assign_io_bump -net DVSS BUMP_6_4
assign_io_bump -net DVDD BUMP_7_3
assign_io_bump -net p_ddr_addr_7_o BUMP_7_1
assign_io_bump -net p_ddr_addr_6_o BUMP_7_0
```

### RDL Routing

To route the RDL for the bump arrays
```
route_rdl -layer layer \
          [-bump_via via] \
          [-pad_via via] \
          [-width width] \
          [-spacing spacing] \
          [-allow45] \
          nets
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layer` | Layer to route on |
| `-bump_via` | Via to use to to connect the bump to the routing layer |
| `-pad_via` | Via to use to to connect the pad cell to the routing layer |
| `-width` | Width of the routing (defaults to minimum width) |
| `-spacing` | Spacing of the routing (defaults to minimum spacing) |
| `-allow45` | Specifies that 45 degree routing is permitted |
| `nets` | Nets to route |

## FAQs

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
