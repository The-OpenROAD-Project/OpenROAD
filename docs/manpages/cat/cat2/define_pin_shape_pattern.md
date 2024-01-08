---
date: 23/12/17
section: 2
title: define\_pin\_shape\_pattern
---

NAME
====

define\_pin\_shape\_pattern - define pin shape pattern

SYNOPSIS
========

define\_pin\_shape\_pattern -layer layer -x\_step x\_step -y\_step
y\_step -region {llx lly urx ury} -size {width height} \[-pin\_keepout
dist\]

DESCRIPTION
===========

The `define_pin_shape_pattern` command defines a pin placement grid on
the specified layer. This grid has positions inside the die area, not
only at the edges of the die boundary.

OPTIONS
=======

`-layer`: The single top-most routing layer of the placement grid.

`-x_step, -y_step`: The distance (in microns) between each valid
position on the grid in the x- and y-directions, respectively.

`-region`: The `{llx, lly, urx, ury}` region of the placement grid (in
microns).

`-size`: The width and height (in microns) of the pins assigned to this
grid. The centers of the pins are placed on the grid positions. Pins may
have half of their shapes outside the defined region.

`-pin_keepout`: The boundary (in microns) around existing routing
obstructions that the pins should avoid; this defaults to the `layer`
minimum spacing.

ARGUMENTS
=========

EXAMPLES
========

ENVIRONMENT
===========

FILES
=====

SEE ALSO
========

HISTORY
=======

BUGS
====

COPYRIGHT
=========

Copyright (c) 2024, The Regents of the University of California. All
rights reserved.

AUTHORS
=======

Jack Luar (TODO\@TODO.com).
