---
title: place_pins(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

place_pins - place pins

# SYNOPSIS

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


# DESCRIPTION

The `place_pins` command places all pins together. Use the following command to perform pin placement:

# OPTIONS

`-hor_layers`:  The layers to create the metal shapes of pins placed in horizontal tracks. It can be a single layer or a list of layer names.

`-ver_layers`:  The layers to create the metal shapes of pins placed in vertical tracks. It can be a single layer or a list of layer names.

`-corner_avoidance`:  The distance (in microns) from each corner within which pin placement should be avoided.

`-min_distance`:  The minimum distance between pins on the die boundary. This distance can be in microns (default) or in number of tracks between each pin.

`-min_distance_in_tracks`:  Flag that allows setting the min distance in number of tracks instead of microns.

`-exclude`:  A region where pins cannot be placed. Either `top|bottom|left|right:edge_interval`, which is the edge interval from the selected edge; `begin:end` for begin-end of all edges.

`-group_pins`:  A list of pins to be placed together on the die boundary.

`-annealing`:  Flag to enable simulated annealing pin placement.

`-write_pin_placement`:  A file with the pin placement generated in the format of multiple calls for the `place_pin` command.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
