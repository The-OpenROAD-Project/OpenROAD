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

`-endcap_master`:  Master used as an endcap.

`-halo_width_x`:  Horizontal halo size (in microns) around macros during cut rows.

`-halo_width_y`:  Vertical halo size (in microns) around macros during cut rows.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
