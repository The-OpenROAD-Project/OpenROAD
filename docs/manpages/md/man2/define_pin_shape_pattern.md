---
title: define_pin_shape_pattern(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/09
---

# NAME

define_pin_shape_pattern - define pin shape pattern

# SYNOPSIS

define_pin_shape_pattern 
    -layer layer
    -x_step x_step
    -y_step y_step
    -region {llx lly urx ury}
    -size {width height}
    [-pin_keepout dist]


# DESCRIPTION

The `define_pin_shape_pattern` command defines a pin placement grid on the
specified layer. This grid has positions inside the die area, not only at
the edges of the die boundary.

# OPTIONS

`-layer`:  The single top-most routing layer of the placement grid.

`-x_step, -y_step`:  The distance (in microns) between each valid position on the grid in the x- and y-directions, respectively.

`-region`:  The `{llx, lly, urx, ury}` region of the placement grid (in microns).

`-size`:  The width and height (in microns) of the pins assigned to this grid. The centers of the pins are placed on the grid positions. Pins may have half of their shapes outside the defined region.

`-pin_keepout`:  The boundary (in microns) around existing routing obstructions that the pins should avoid; this defaults to the `layer` minimum spacing.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
