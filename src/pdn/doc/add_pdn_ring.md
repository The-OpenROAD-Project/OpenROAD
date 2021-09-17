# add_pdn_ring

## Synopsis
```
  % add_pdn_ring
    [-grid grid_name] \
    -layers layer_name \
    -widths (width_value|list_of_2_values) \
    -spacings (spacing_value|list_of_2_values) \
    [-core_offset offset_value] \
    [-pad_offset offset_value] \
    [-power_pads list_of_core_power_pads]
    [-ground_pads list_of_core_ground_pads]
```

## Description

The `add_pnd_ring` command is used to define power/ground rings around a stdcell region. The ring structure is built using two layers that are orthogonal to each other. A power/ground pair will be added above and below the grid using the horizontal layer, with another power/ground pair to the left and right using the vertical layer. Together these 4 pairs of power/ground stripes form a ring around the specified grid. Power straps on these layers that are inside the enclosed region are extend to connect to the ring.

The `-grid` argument defines the name of the grid for which this ring specification will be added. If no `-grid` argument is specified, the pattern will be added to the grid created with the previous [define_pdn_grid](define_pdn_grid.md) command.
The `-layers` argument specifies the two orthogonal layers to be used for the ring.
The `-widths` argument can either be a single value, which is used as the width for both layers, or else a list of two values to be used as widths for their respective layers.
The `-spacings` argument can either be a single value, which is used as the spacing between power/ground stripes for both layers, or else a list of two values to be used as the spacing for their respective layers.
The `-core_offset` argument is used to specify the spacing from the grid region to the rings. Alternatively, the `-pad_offset` argument can be used to specify a distance from the edges of the pad cells for power/ground rings at the top level of the SoC.
The `-power_pads` and `-ground_pads` options are used only when the `-pad_offset` option is specified and are used to identify the padcells that the ring is placed relative to. In addition, core side power and ground pins on instances of these padcells are connected to the ring. Where there would be a conflict between connections from the core, and connections from the padcells, the padcell connections will take precedence.

## Options

| Switch Name | Description |
| ----- | ----- |
| `-grid` | Specifies the name of the grid to which this ring defintion will be added. (Default: Last grid created by defin_pdn_grid)|
| `-layer` | Specifies the name of the layer for these stripes |
| `-width` | Value for the width of the stdcell rail |
| `-spacing` | Optional specification of the spacing between power/ground pairs within a single pitch. (Default: pitch / 2) |
| `-core_offset` | Value for the offset of the ring from the grid region |
| `-pad_offset` | When defining a power grid for the top level of an SoC, can be used to define the offset of ring from the pad cells |
| `-power_pads` | A list of core power pad cells. The core side power pin of these padcell instances will be connected to the ring |
| `-ground_pads` | A list of core ground pad cells. The core side ground pin of these padcell instances will be connected to the ring |


## Examples
```
add_pdn_ring   -grid main_grid -layer {metal6 metal7} -widths 5.0 -spacings  3.0 -core_offset 5
```
