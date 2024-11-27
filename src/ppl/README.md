# Pin Placer

Place pins on the boundary of the die on the track grid to minimize net
wirelengths. Pin placement also creates a metal shape for each pin using
min-area rules.

For designs with unplaced cells, the net wirelength is computed considering
the center of the die area as the unplaced cells position.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Define Pin Shape Pattern

The `define_pin_shape_pattern` command defines a pin placement grid on the
specified layer. This grid has positions inside the die area, not only at
the edges of the die boundary.

```tcl
define_pin_shape_pattern 
    [-layer layer]
    [-x_step x_step]
    [-y_step y_step]
    [-region {llx lly urx ury} | *]
    [-size {width height}]
    [-pin_keepout dist]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layer` | The single top-most routing layer of the placement grid. |
| `-x_step`, `-y_step` | The distance (in microns) between each valid position on the grid in the x- and y-directions, respectively. |
| `-region` | The `{llx, lly, urx, ury}` region of the placement grid (in microns). If the `*` is specified, the region will be the entire die area. |
| `-size` | The width and height (in microns) of the pins assigned to this grid. The centers of the pins are placed on the grid positions. Pins may have half of their shapes outside the defined region. |
| `-pin_keepout` | The boundary (in microns) around existing routing obstructions that the pins should avoid; this defaults to the `layer` minimum spacing. |

#### Face-to-Face direct-bonding IOs

The `define_pin_shape_pattern` command can be used to place pins in any metal
layer with the minimum allowed spacing to facilitate 3DIC integration of
chips using face-to-face packaging technologies. These technologies include
[micro bumps](https://semiengineering.com/bumps-vs-hybrid-bonding-for-advanced-packaging/)
and
[hybrid bonding](https://www.3dincites.com/2018/04/hybrid-bonding-from-concept-to-commercialization/)
for high density face-to-face interconnect.

### Set IO Pin Constraints

The `set_io_pin_constraint` command sets region constraints for pins according
to the pin direction or the pin name. This command can be called multiple
times with different constraints.

You can use the `set_io_pin_constraint` command to restrict pins to the
pin placement grid created with the `define_pin_shape_pattern` command.

It is possible to use the `-region`, `-group` and `-order` arguments together
per `set_io_pin_constraint` call, but the `-mirrored_pins` argument should be
called alone.

```tcl
set_io_pin_constraint 
    [-direction direction]
    [-pin_names names]
    [-region edge:interval]
    [-mirrored_pins names]
    [-group]
    [-order]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-direction` | Pin direction (`input`, `output`, `inout`, or `feedthrough`). |
| `-pin_names` | List of names. Only one of (`-direction`, `-pin_names`) should be used in a single call for the `set_io_pin_constraint` command. |
| `-region` | Syntax is `-region edge:interval`. The `edge` values are (`top\|bottom\|left\|right`). The `interval` can be the whole edge with the wildcard `*` value or a range of values. |
| `-mirrored_pins` | List of pins that sets pairs of pins that will be symmetrically placed in the vertical or the horizontal edges. The number of pins in this list **must be even**. For example, in `set_io_pin_constraint -mirrored_pins {pin1 pin2 pin3 pin4 pin5 pin6}`, the pins `pin1` and `pin2` will be placed symmetrically to each other. Same for `pin3` and `pin4`, and for `pin5` and `pin6`. |
| `-group` | Flag places together on the die boundary the pin list defined in `-pin_names,` similar to the `-group_pins` option on the `place_pins` command. |
| `-order` | Flag places the pins ordered in ascending x/y position and must be used only when `-group` is also used. |

The `edge` values are (up, top, bottom, left, right), where `up` is
the grid created by `define_pin_shape_pattern`. To restrict pins to the
pin placement grid defined with `define_pin_shape_pattern` use:

-   `-region up:{llx lly urx ury}` to restrict the pins into a specific
    region in the grid. The region is defined in microns.
-   `-region up:*` to restrict the pins into the entire region of the grid.

The `up` option is only available when the pin placement grid is created with
the `define_pin_shape_pattern` command.

### Clear IO Pin Constraints

The `clear_io_pin_constraints` command clears all the previously-defined
constraints and pin shape patterns created with `set_io_pin_constraint` or
`define_pin_shape_pattern`.

```tcl
clear_io_pin_constraints
```

### Set Pin Length

The `set_pin_length` command defines the length of all vertical and horizontal
pins.

```tcl
set_pin_length 
    [-hor_length h_length]
    [-ver_length v_length]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-hor_length` | The length (in microns) of the horizontal pins. |
| `-ver_length` | The length (in microns) of the vertical pins. |

The default length of the pins is the minimum length necessary to respect the
minimum area defined in the routing layer they were placed. The width of the
pins is the minimum width defined in the routing layer.

### Set Pin Length Extension

The `set_pin_length_extension` command defines the an extension of the length
of all vertical and horizontal pins. Note that this command may generate pins
partially outside the die area.

```tcl
set_pin_length_extension 
    [-hor_extension h_extension]
    [-ver_extension v_extension]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-hor_extension` | The length (in microns) for the horizontal pins. |
| `-ver_extension` | The length (in microns) for the vertical pins. |

### Set Pin Thickness Multiplier

The `set_pin_thick_multiplier` command defines a multiplier for the thickness of all
vertical and horizontal pins.

```tcl
set_pin_thick_multiplier 
    [-hor_multiplier h_mult]
    [-ver_multiplier v_mult]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-hor_multiplier` | The thickness multiplier for the horizontal pins. |
| `-ver_multiplier` | The thickness multiplier for the vertical pins. |

### Set Simulated Annealing

The `set_simulated_annealing` command defines the parameters for simulated annealing pin placement.

```tcl
set_simulated_annealing
    [-temperature temperature]
    [-max_iterations iter]
    [-perturb_per_iter perturbs]
    [-alpha alpha]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-temperature` | Temperature parameter. The default value is `1.0`, and the allowed values are floats `[0, MAX_FLOAT]`. |
| `-max_iterations` | The maximum number of iterations. The default value is `2000`, and the allowed values are integers `[0, MAX_INT]`. |
| `-perturb_per_iter` | The number of perturbations per iteration. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`. |
| `-alpha` | The temperature decay factor. The default value is `0.985`, and the allowed values are floats `(0, 1]`. |

### Simulated Annealing Debug Mode

The `simulated_annealing_debug` command allows you to debug the simulated
annealing pin placement with a pause mode.

```tcl
simulated_annealing_debug
    [-iters_between_paintings iters]
    [-no_pause_mode no_pause_mode]
```

#### Options

| Switch Name | Description |
| ----- | ----- |                                    
| `-iters_between_paintings` | Determines the number of iterations between updates. |
| `-no_pause_mode` | Print solver state every second based on `iters_between_paintings`. |
                                    
### Place specific Pin

The `place_pin` command places a specific pin in the specified location with the specified size.
It is recommended that individual pins be placed before the `place_pins` command,
as the routing tracks occupied by these individual pins will be blocked, preventing overlaps.

To place an individual pin:

```tcl
place_pin 
    -pin_name pin_name
    -layer layer
    -location {x y}
    [-pin_size {width height}]
    [-force_to_die_boundary]
    [-placed_status]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-pin_name` | The name of a pin of the design. |
| `-layer` | The routing layer where the pin is placed. |
| `-location` | The center of the pin (in microns). |
| `-pin_size` | The width and height of the pin (in microns). |
| `-force_to_die_boundary` | When this flag is enabled, the pin will be snapped to the nearest routing track, next to the die boundary. |
| `-placed_status` | When this flag is enabled, the pin will have PLACED as its placement status, instead of the FIXED status. |

### Place all Pins

The `place_pins` command places all pins together. Use the following command to perform pin placement:

Developer arguments:
- `-random`, `-random_seed`

```tcl
place_pins 
    -hor_layers h_layers
    -ver_layers v_layers
    [-random_seed seed]
    [-random]
    [-corner_avoidance length]
    [-min_distance distance]
    [-min_distance_in_tracks]
    [-exclude region]
    [-group_pins pin_list]
    [-annealing]
    [-write_pin_placement file_name]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-hor_layers` | The layers to create the metal shapes of pins placed in horizontal tracks. It can be a single layer or a list of layer names. |
| `-ver_layers` | The layers to create the metal shapes of pins placed in vertical tracks. It can be a single layer or a list of layer names. |
| `-corner_avoidance` | The distance (in microns) from each corner within which pin placement should be avoided. |
| `-min_distance` | The minimum distance between pins on the die boundary. This distance can be in microns (default) or in number of tracks between each pin. The default value is the length of two routing tracks between each pin. |
| `-min_distance_in_tracks` | Flag that allows setting the min distance in number of tracks instead of microns. |
| `-exclude` | A region where pins cannot be placed. Either `top|bottom|left|right:edge_interval`, which is the edge interval from the selected edge; `begin:end` for begin-end of all edges. |
| `-group_pins` | A list of pins to be placed together on the die boundary. |
| `-annealing` | Flag to enable simulated annealing pin placement. |
| `-write_pin_placement` | A file with the pin placement generated in the format of multiple calls for the `place_pin` command. |

The `exclude` option syntax is `-exclude edge:interval`. The `edge` values are
(top|bottom|left|right). The `interval` can be the whole edge, with the `*`
value, or a range of values. For example, in `place_pins -hor_layers metal2
-ver_layers metal3 -exclude top:* -exclude right:15-60.5 -exclude left:*-50`
three intervals are excluded: the whole top edge, the right edge from 15
microns to 60.5 microns, and the left edge from its beginning to 50 microns.

#### Developer Arguments

| Switch Name | Description |
| ----- | ----- |
| `-random_seed` | Specify the seed for random operations. |
| `-random` | When this flag is enabled, the pin placement is random. |

### Write Pin Placement

The `write_pin_placement` command writes a file with the pin placement in the format of multiple calls for the `place_pin` command:

```tcl
write_pin_placement 
    file_name
    -placed_status
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `file_name` | The name of the file with the pin placement. |
| `-placed_status` | When this flag is enabled, the file will be generated´with the `-placed_status` flag in each `place_pin` command call. |

## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/IOPlacer.cpp) or the [swig file](./src/IOPlacer.i).

| Command Name | Description |
| ----- | ----- |
| `parse_edge` | Parse edge (top/bottom/left/right). |
| `parse_direction` | Parse direction. |
| `parse_excludes_arg` | Parse excluded arguments. |
| `parse_group_pins_arg` | Parse group pins arguments. | 
| `parse_layer_name` | Parse layer name. |
| `parse_pin_names` | Parse pin names. |
| `get_edge_extreme` | Get extremes of edge. |
| `exclude_intervals` | Set exclude interval. |
| `add_pins_to_constraint` | Add pins to constrained region. |
| `add_pins_to_top_layer` | Add pins to top layer. | 

## Example scripts

Example scripts of `ppl` running on a sample design of `gcd` as follows:

```
./test/gcd.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## References

- This code depends on [Munkres](src/munkres/README.txt).

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+ioplacer+in%3Atitle)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
