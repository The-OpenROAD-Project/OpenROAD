## add_pdn_ring

### Synopsis
```
  % add_pdn_ring
    [-name grid_name] \
    -layer layer_name \
    -width width_value \
    -spacing spacing_value \
    [-core_offset offset_value] \
    [-pad_offset offset_value] 
```

### Description

The add_pnd_ring command is used to define power/ground rings around a stdcell region. The ring structure is built using two add_pdn_ring commands using layers that are orthogonal to each other. Each add_pdn_ring command will place a power/ground pair above and below, in the case of a horizontal layer, or left and right, in the case of a vertical layer. Together these 4 pairs of power/ground stripes form a ring around the specified grid. i

The -name argument defines the name of the grid for which this ring specification will be added. If no -name argument is specified, the pattern will be added to the grid created with the previous [define_pdn_grid](define_pdn_grid.md) command.
The -layer argument specifies the layer to be used for the ring.
The -width argument defines the width of the layer in the ring.
The -spacing argument specifies the spacing between power/ground stripes that form the ring
The -core_offset argument is used to specify the spacing from the grid region to the rings. Alternatively, the -pad_offset argument can be used to specify a distance from the edges of the pad cells for power/ground rings at the top level of the SoC.


### Options

| Switch Name | Description |
| ----- | ----- |
| -name | Defines a name to use when referring to this grid definition |
| -layer | Specifies the name of the layer for these stripes |
| -width | Value for the width of the stdcell rail |
| -spacing | Optional specification of the spacing between power/ground pairs within a single pitch. (Default: pitch / 2) |
| -core_offset | Value for the offset of the ring from the grid region |
| -pad_offset | When defining a power grid for the top level of an SoC, can be used to define the offset of ring from the pad cells |

### Examples
```
add_pdn_ring   -name main_grid -layer metal6 -width 5.0 -spacing  3.0 -core_offset 5
add_pdn_ring   -name main_grid -layer metal7 -width 5.0 -spacing  3.0 -core_offset 5
```

