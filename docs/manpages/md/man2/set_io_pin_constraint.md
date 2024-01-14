---
title: set_io_pin_constraint(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

set_io_pin_constraint - set io pin constraint

# SYNOPSIS

set_io_pin_constraint 
    [-direction direction]
    [-pin_names names]
    [-region edge:interval]
    [-mirrored_pins names]
    [-group]
    [-order]


# DESCRIPTION

The `set_io_pin_constraint` command sets region constraints for pins according
to the pin direction or the pin name. This command can be called multiple
times with different constraints.

You can use the `set_io_pin_constraint` command to restrict pins to the
pin placement grid created with the `define_pin_shape_pattern` command.

It is possible to use the `-region`, `-group` and `-order` arguments together
per `set_io_pin_constraint` call, but the `-mirrored_pins` argument should be
called alone.

# OPTIONS

`-direction`:  Pin direction (`input`, `output`, `inout`, or `feedthrough`).

`-pin_names`:  List of names. Only one of (`-direction`, `-pin_names`) should be used in a single call for the `set_io_pin_constraint` command.

`-region`:  Syntax is `-region edge:interval`. The `edge` values are (`top|bottom|left|right`). The `interval` can be the whole edge with the wildcard `*` value or a range of values.

`-mirrored_pins`:  List of pins that sets pairs of pins that will be symmetrically placed in the vertical or the horizontal edges. The number of pins in this list **must be even**. For example, in `set_io_pin_constraint -mirrored_pins {pin1 pin2 pin3 pin4 pin5 pin6}`, the pins `pin1` and `pin2` will be placed symmetrically to each other. Same for `pin3` and `pin4`, and for `pin5` and `pin6`.

`-group`:  Flag places together on the die boundary the pin list defined in `-pin_names,` similar to the `-group_pins` option on the `place_pins` command.

`-order`:  Flag places the pins ordered in ascending x/y position and must be used only when `-group` is also used.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
