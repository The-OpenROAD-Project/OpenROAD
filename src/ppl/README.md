# ioPlacer

Place pins on the boundary of the die on the track grid to minimize net
wirelengths. Pin placement also creates a metal shape for each pin using
min-area rules.

For designs with unplaced cells, the net wirelength is computed considering
the center of the die area as the unplaced cells' position.

## Commands

### Place All Pins

Use the following command to perform pin placement:

```
place_pins [-hor_layers <h_layers>]
           [-ver_layers <v_layers>]
           [-random_seed <seed>]
           [-exclude <interval>]
           [-random]
           [-group_pins <pins>]
           [-corner_avoidance <length>]
           [-min_distance <distance>]
           [-min_distance_in_tracks]
```

-   `-hor_layers` (mandatory). Specify the layers to create the metal shapes
    of pins placed in horizontal tracks. Can be a single layer or a list
    of layer names.
-   `-ver_layers` (mandatory). Specify the layers to create the metal
    shapes of pins placed in vertical tracks. Can be a single layer or a
    list of layer names.
-   `-random_seed`. Specify the seed for random operations.
-   `-exclude`. Specify an interval in one of the four edges of the die
    boundary where pins cannot be placed. Can be used multiple times.
-   `-random`. When this flag is enabled, the pin placement is random.
-   `-group_pins`. Specify a list of pins to be placed together on the
    die boundary.
-   `-corner_avoidance distance`. Specify the distance (in microns) from
    each corner within which pin placement should be avoided.
-   `-min_distance distance`. Specify the minimum distance between pins on
    the die boundary.  This distance can be in microns (default) or in number of tracks
    between each pin.
-   `-min_distance_in_tracks`. Flag that allows setting the min distance in
    number of tracks instead of microns.

The `exclude` option syntax is `-exclude edge:interval`. The `edge` values are
(top|bottom|left|right). The `interval` can be the whole edge, with the `*`
value, or a range of values. For example, in `place_pins -hor_layers metal2
-ver_layers metal3 -exclude top:* -exclude right:15-60.5 -exclude left:*-50`
three intervals are excluded: the whole top edge, the right edge from 15
microns to 60.5 microns, and the left edge from its beginning to 50 microns.

### Place Individual Pin

```
place_pin [-pin_name <pin_name>]
          [-layer <layer>]
          [-location <{x y}>]
          [-pin_size <{width height}>]
```

The `place_pin` command places a specific pin in the specified location, with the specified size.
-   `-pin_name` option is the name of a pin of the design.
-   `-layer` defines the routing layer where the pin is placed.
-   `-location` defines the center of the pin.
-   `-pin_size` option defines the width and height of the pin.


### Define Pin Shape Pattern

The `define_pin_shape_pattern` command defines a pin placement grid on the
specified layer. This grid has positions inside the die area, not only at
the edges of the die boundary.

```
define_pin_shape_pattern [-layer <layer>]
                         [-x_step <x_step>]
                         [-y_step <y_step>]
                         [-region <{llx lly urx ury}>]
                         [-size <{width height}>]
                         [-pin_keepout <dist>]
```

-   The `-layer` option defines a single top-most routing layer of the
    placement grid.
-   The `-region` option defines the `{llx, lly, urx, ury}` region of the
    placement grid.
-   The `-x_step` and `-y_step` options define the distance between each
    valid position on the grid, in the x- and y-directions, respectively.
-   The `-size` option defines the width and height of the pins assigned
    to this grid. The centers of the pins are placed on the grid
    positions. Pins may have half of their shapes outside the defined region.
-   The `-pin_keepout` option defines the boundary (microns) around
    existing routing obstructions that the pins should avoid; this defaults to the
    `layer` minimum spacing.


### Set IO Pin Constraint

The `set_io_pin_constraint` command sets region constraints for pins according
to the pin direction or the pin name. This command can be called multiple
times with different constraints. Only one condition should be used for
each command call.

The `-direction` argument is the pin direction (input, output, inout, or
feedthrough). The `-pin_names` argument is a list of names. The `-region`
syntax is the same as that of the `-exclude` syntax.

#### Face-to-Face direct-bonding IOs

Studies have shown that 3D integration offers power and performance
benefit.  There is also a newly emerging opportunity for 3D-IC
application in the area of design obfuscation.
[Micro bumps and hybrid bonding](https://semiengineering.com/bumps-vs-hybrid-bonding-for-advanced-packaging/)
are two of the more advanced packaging technologies that
are used for 3D integration via face-to-face stacking. The
[hybrid bonding](https://www.3dincites.com/2018/04/hybrid-bonding-from-concept-to-commercialization/)
technology provides even denser interconnect for the
[3D stack](https://en.wikipedia.org/wiki/Direct_bonding). The implementation
of face-to-face stacking involves placement of the ports/pins of the
individual chips in the die area during physical design steps.

Note that if you call `define_pin_shape_pattern` before
`set_io_pin_constraint`, the `edge` values are (up, top,
bottom, left, right). Where `up` relates to the layer created by
`define_pin_shape_pattern`. To restrict pins to the pin placement grid
defined with `define_pin_shape_pattern` use:

-   `-region up:{llx lly urx ury}` to restrict the pins into a specific
    region in the grid
-   `-region up:*` to restrict the pins into the entire region of the grid.


```
set_io_pin_constraint -direction <direction>
                      -pin_names <names>
                      -region <edge:interval>
```

### Clear IO Pin Constraints

The `clear_io_pin_constraints` command clears all the previously-defined
constraints and pin shape patterns created with `define_pin_shape_pattern`.

```
clear_io_pin_constraints
```

## Example scripts

## Regression tests

## Limitations

## External references

- This code depends on [Munkres](src/munkres/README.md).

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+ioplacer+in%3Atitle)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
