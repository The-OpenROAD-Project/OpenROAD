## define_pdn_grid

### Synopsis
For specifying a power grid over the stdcell area
```
  % define_pdn_grid
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
  % define_pdn_grid
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

### Description

### Options

| Switch Name | Description |
| ----- | ----- |
| -type | Defines the type of grid being added, can be either stdcell or macro|
| -name | Defines a name to use when referring to this grid definition |
| -rails | Defines a list of rail specifications to define the followpins connections to the stdcells |
| -straps | Defines a list of strap specifications to define the power straps to be added to the design |
| -pins | Defines a list of layers which where the power straps will be promoted to block pins |
| -connect | Defines the connections to be made between the layers |
| -starts_with | Specifies whether the first strap placed will be POWER or GROUND |
| -orient | For a macro, defines a set of valid orientations. Macros with one of the valid orientations will use this grid specification |
| -blockages | For a macro, defines which layers are to be treated as blockages for the stdcell grid |
| -power_pins | For a macro, define the names of pins on the macro to be treated as power pins |
| -ground_pins | For a macro, define the names of pins on the macro to be treated as ground pins |


### Specifications

#### Rail specifications
A rails specification defines the width and offset of the stdcell rails for each layer that forms the stdcell followpins structure.
| Attribute | Description |
| ----- | ----- |
| width | Value for the width of the stdcell rail |
| offset | Value for the offset of the stdcell rail |

#### Strap specifications
A strap specification defines the width, pitch, offset and spacing for each layer that will have power straps added. The spacing attribute is optional, and defaults to half the pitch.
| Attribute | Description |
| ----- | ----- |
| width | Value for the width of the power strap |
| offset | Value for the offset of the power strap |
| pitch | Value for the distance between each power/ground pair |
| spacing | Optional specification of the spacing between power/ground pairs within a single pitch. (Default: pitch / 2) |
| starts_with | Optional specifies whether the first stripe is POWER or GROUND (Default: POWER) |

#### Connection specifications
A connection specification defines which layers are to be connected to each other. This consists of a pair of layer names, optionally followed by some constraint settings

For macro grids, to consider the pins of the macro as power/ground straps on that layer, add _PIN to the layer name.

### Examples
```
define_pdn_grid -name grid \
  -type stdcell \
  -rails {
    metal1 {width 0.17 offset 0}
    metal2 {width 0.17 offset 0}
   } \
  -straps {
    metal4 {width 0.48 pitch 56.0 offset 2}
    metal7 {width 1.40 pitch 40.0 offset 2 -starts_with POWER}
   } \
  -connect {{metal1 metal2 constraints {cut_pitch 0.16}} {metal2 metal4} {metal4 metal7}} \
  -pins metal7 \
  -starts_with POWER

define_pdn_grid \
  -name ram \
  -type macro \
  -orient {R0 R180 MX MY} \
  -power_pins "VDD VDDPE VDDCE" \
  -ground_pins "VSS VSSE" \
  -blockages "metal1 metal2 metal3 metal4 metal5 metal6" \
  -straps {
    metal5 {width 0.93 pitch 10.0 offset 2}
    metal6 {width 0.93 pitch 10.0 offset 2}
   } \
  -connect {{metal4_PIN_ver metal5} {metal5 metal6} {metal6 metal7}} \
  -starts_with POWER

```

