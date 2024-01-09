---
title: place_pin(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/09
---

# NAME

place_pin - place pin

# SYNOPSIS

place_pin 
    -pin_name pin_name
    -layer layer
    -location {x y}
    [-pin_size {width height}]
    [-force_to_die_boundary]


# DESCRIPTION

The `place_pin` command places a specific pin in the specified location with the specified size.
It is recommended that individual pins be placed before the `place_pins` command,
as the routing tracks occupied by these individual pins will be blocked, preventing overlaps.

To place an individual pin:

# OPTIONS

`-pin_name`:  The name of a pin of the design.

`-layer`:  The routing layer where the pin is placed.

`-location`:  The center of the pin (in microns).

`-pin_size`:  Tthe width and height of the pin (in microns).

`-force_to_die_boundary`:  When this flag is enabled, the pin will be snapped to the nearest routing track, next to the die boundary.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
