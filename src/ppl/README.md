# ioPlacer

Place pins on the boundary of the die on the track grid to
minimize net wire lengths. Pin placement also
creates a metal shape for each pin using min-area rules.

For designs with unplaced cells, the net wire length is
computed considering the center of the die area as the
unplaced cells' position.

Use the following command to perform pin placement:
```
place_pins [-hor_layers h_layers]
           [-ver_layers v_layers]
           [-random_seed seed]
           [-exclude interval]
           [-random]
           [-group_pins pins]
           [-corner_avoidance length]
           [-min_distance distance]
           [-min_distance_in_tracks]
```

- ``-hor_layers`` (mandatory). Specify the layers to create the metal shapes
of pins placed in horizontal tracks. Can be a single layer or a list of layer names.
- ``-ver_layers`` (mandatory). Specify the layers to create the metal shapes
of pins placed in vertical tracks. Can be a single layer or a list of layer names.
- ``-random_seed``. Specify the seed for random operations.
- ``-exclude``. Specify an interval in one of the four edges of the die boundary
where pins cannot be placed. Can be used multiple times.
- ``-random``. When this flag is enabled, the pin placement is
random.
- ``-group_pins``. Specify a list of pins to be placed together on the die boundary.
- ``-corner_avoidance distance``. Specify the distance (in micron) from each corner to avoid placing pins.
- ``-min_distance distance``. Specify the minimum distance between pins in the die boundary.
It can be in microns (default) or in number of tracks between each pin.
- ``-min_distance_in_tracks``. Flag that allows set the min distance in number of tracks instead of microns.

The `exclude` option syntax is `-exclude edge:interval`. The `edge` values are
(top|bottom|left|right). The `interval` can be the whole edge, with the `*` value,
or a range of values. Example: `place_pins -hor_layers metal2 -ver_layers metal3 -exclude top:* -exclude right:15-60.5 -exclude left:*-50`.
In the example, three intervals were excluded: the whole top edge, the right edge from 15 microns to 60.5 microns, and the
left edge from the beginning to the 50 microns.

```
place_pin [-pin_name pin_name]
          [-layer layer]
          [-location {x y}]
          [-pin_size {width height}]
```

The `place_pin` command places a specific pin in the specified location, with the specified size.
The `-pin_name` option is the name of a pin of the design.
The `-layer` defines the routing layer where the pin is placed.
The `-location` defines the center of the pin.
The `-pin_size` option defines the width and height of the pin.

```
define_pin_shape_pattern [-layer layer]
                         [-x_step x_step]
                         [-y_step y_step]
                         [-region {llx lly urx ury}]
                         [-size {width height}]
                         [-pin_keepout dist]
```

The `define_pin_shape_pattern` command defines a pin placement grid at the specified layer.
This grid has positions inside the die area, not only at the edges of the die boundary.
The `-layer` option defines a single top most routing layer of the placement grid.
The `-region` option defines the {llx, lly, urx, ury} region of the placement grid.
The `-x_step` and `-y_step` options define the distance between each valid position on the grid.
The `-size` option defines the width and height of the pins assigned to this grid. The center of the
pins are placed on the grid positions. Pins may have half of their shapes outside the defined region.
The `-pin_keepout` option defines the boundary (microns) around existing routing obstructions the pins should avoid, defaults to the `layer` minimum spacing.

```
set_io_pin_constraint -direction direction -pin_names names -region edge:interval
```

The `set_io_pin_constraint` command sets region constraints for pins according the direction or the pin name.
This command can be called multiple times with different constraints. Only one condition should be used for each
command call. The `-direction` argument is the pin direction (input, output, inout, or feedthrough).
The `-pin_names` argument is a list of names. The `-region` syntax is the same as the `-exclude` syntax.
To restrict pins to the positions defined with `define_pin_shape_pattern`, use `-region up:{llx lly urx ury}` or `-region up:*`.

```
clear_io_pin_constraints
```

The `clear_io_pin_constraints` command clear all the previous defined constraints and pin shape pattern for top layer placement.

# Authors

Copyright (c) 2019--2020, Federal University of Rio Grande do Sul (UFRGS)
All rights reserved.
