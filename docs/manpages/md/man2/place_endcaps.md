---
title: place_endcaps(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

place_endcaps - place endcaps

# SYNOPSIS

place_endcaps
    [-corner master]
    [-edge_corner master]
    [-endcap masters]
    [-endcap_horizontal masters]
    [-endcap_vertical master]
    [-prefix prefix]
    [-left_top_corner master]
    [-right_top_corner master]
    [-left_bottom_corner master]
    [-right_bottom_corner master]
    [-left_top_edge master]
    [-right_top_edge master]
    [-left_bottom_edge master]
    [-right_bottom_edge master]
    [-left_edge master]
    [-right_edge master]
    [-top_edge masters]
    [-bottom_edge masters]


# DESCRIPTION

Place endcaps into the design, the naming for the arguments to `place_endcaps` is based on the 
LEF58 `CLASS` specification foe endcaps.

# OPTIONS

`-prefix`:  Prefix to use for the boundary cells. Defaults to "PHY_".

`-corner`:  Master for the corner cells on the outer corners.

`-edge_corner`:  Master for the corner cells on the inner corners.

`-endcap`:  Master used as an endcap.

`-endcap_horizontal`:  List of masters for the top and bottom row endcaps. (overrides `-endcap`).

`-endcap_vertical`:  Master for the left and right row endcaps. (overrides `-endcap`).

`-left_top_corner`:  Master for the corner cells on the outer top left corner. (overrides `-corner`).

`-right_top_corner`:  Master for the corner cells on the outer top right corner. (overrides `-corner`).

`-left_bottom_corner`:  Master for the corner cells on the outer bottom left corner. (overrides `-corner`).

`-right_bottom_corner`:  Master for the corner cells on the outer bottom right corner. (overrides `-corner`).

`-left_top_edge`:  Master for the corner cells on the inner top left corner. (overrides `-edge_corner`).

`-right_top_edge`:  Master for the corner cells on the inner top right corner. (overrides `-edge_corner`).

`-left_bottom_edge`:  Master for the corner cells on the inner bottom left corner. (overrides `-edge_corner`).

`-right_bottom_edge`:  Master for the corner cells on the inner bottom right corner. (overrides `-edge_corner`).

`-left_edge`:  Master for the left row endcaps. (overrides `-endcap_vertical`).

`-right_edge`:  Master for the right row endcaps. (overrides `-endcap_vertical`).

`-top_edge`:  List of masters for the top row endcaps. (overrides `-endcap_horizontal`).

`-bottom_edge`:  List of masters for the bottom row endcaps. (overrides `-endcap_horizontal`).

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
