# add_pdn_stripe

## Synopsis
```
  % add_pdn_stripe
    [-grid grid_name] \
    -layer layer_name \
    -width width_value \
    [-pitch pitch_value] \
    [-spacing spacing_value] \
    [-offset offset_value] \
    [-starts_with (POWER|GROUND)] \
    [-followpins]
```

## Description

Defines a pattern of power and ground stripes in a single layer to be added to a power grid.

The `-grid` argument defines the name of the grid to which this stripe specification will be added. If no `-grid` argument is specified, the pattern will be added to the grid created with the previous [define_pdn_grid](define_pdn_grid.md) command.

The `-layer` argument specifies the layer to be used for the specified stripe pattern.
The `-width` argument defines the width of the stripes.
The `-pitch` argument defines the pitch of pairs of power/ground stripes.
The `-spacing` argument specifies the spacing between power/ground stripes within a single pitch. (Default: pitch / 2)
The `-offset` argument specifies the distance from the edge of the block to the first power/ground stripe.
The `-starts_with` argument is used to specifiy whether the first stripe is POWER or GROUND. (Default: GROUND)
The `-followpins` flag defines the stripes to be stdcell rails. When `-followpins` is used the values for pitch and spacing are determined from the pattern of stdcell rows.


## Options

| Switch Name | Description |
| ----- | ----- |
| `-grid` | Specifies the grid to which this stripe definition will be added. (Default: Last grid defined by `define_pdn_grid`) |
| `-layer` | Specifies the name of the layer for these stripes |
| `-width` | Value for the width of the stdcell rail |
| `-pitch` | Value for the distance between each power/ground pair |
| `-spacing` | Optional specification of the spacing between power/ground pairs within a single pitch. (Default: pitch / 2) |
| `-offset` | Value for the offset of the stdcell rail |
| `-starts_with` | Specifies whether the first strap placed will be POWER or GROUND (Default: GROUND) |
| `-followpins` | Indicates that the stripe forms part of the stdcell rails, pitch and spacing are dictated by the stdcell rows |

## Examples
```
add_pdn_stripe -grid main_grid -layer metal1 -width 0.17 -followpins
add_pdn_stripe -grid main_grid -layer metal2 -width 0.17 -followpins
add pdn_stripe -grid main_grid -layer metal4 -width 0.48 -pitch 56.0 -offset 2 -starts_with POWER
```
