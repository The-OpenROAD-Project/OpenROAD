---
title: write_pin_placement(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

write_pin_placement - write pin placement

# SYNOPSIS

write_pin_placement 
    file_name


# DESCRIPTION

The `write_pin_placement` command writes a file with the pin placement in the format of multiple calls for the `place_pin` command:

# OPTIONS

`file_name`:  The name of the file with the pin placement.

`Command Name`:  Description

`parse_edge`:  Parse edge (top/bottom/left/right).

`parse_direction`:  Parse direction.

`parse_excludes_arg`:  Parse excluded arguments.

`parse_group_pins_arg`:  Parse group pins arguments.

`parse_layer_name`:  Parse layer name.

`parse_pin_names`:  Parse pin names.

`get_edge_extreme`:  Get extremes of edge.

`exclude_intervals`:  Set exclude interval.

`add_pins_to_constraint`:  Add pins to constrained region.

`add_pins_to_top_layer`:  Add pins to top layer.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
