---
date: 23/12/17
section: 2
title: place\_pin
---

NAME
====

place\_pin - place pin

SYNOPSIS
========

place\_pin -pin\_name pin\_name -layer layer -location {x y}
\[-pin\_size {width height}\] \[-force\_to\_die\_boundary\]

DESCRIPTION
===========

The `place_pin` command places a specific pin in the specified location
with the specified size. It is recommended that individual pins be
placed before the `place_pins` command, as the routing tracks occupied
by these individual pins will be blocked, preventing overlaps.

To place an individual pin:

OPTIONS
=======

`-hor_layers`: The layers to create the metal shapes of pins placed in
horizontal tracks. It can be a single layer or a list of layer names.

`-ver_layers`: The layers to create the metal shapes of pins placed in
vertical tracks. It can be a single layer or a list of layer names.

`-corner_avoidance`: The distance (in microns) from each corner within
which pin placement should be avoided.

`-min_distance`: The minimum distance between pins on the die boundary.
This distance can be in microns (default) or in number of tracks between
each pin.

`-min_distance_in_tracks`: Flag that allows setting the min distance in
number of tracks instead of microns.

`-exclude`: A region where pins cannot be placed. Either
`top|bottom|left|right:edge_interval`, which is the edge interval from
the selected edge; `begin:end` for begin-end of all edges.

`-group_pins`: A list of pins to be placed together on the die boundary.

`-annealing`: Flag to enable simulated annealing pin placement.

`-write_pin_placement`: A file with the pin placement generated in the
format of multiple calls for the `place_pin` command.

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
