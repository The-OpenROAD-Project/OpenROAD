---
title: macro_placement(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/11
---

# NAME

macro_placement - macro placement

# SYNOPSIS

macro_placement 
    [-halo {halo_x halo_y}]
    [-channel {channel_x channel_y}]
    [-fence_region {lx ly ux uy}]
    [-snap_layer snap_layer_number]
    [-style corner_wax_wl|corner_min_wl]


# DESCRIPTION

This command performs macro placement.
For placement style, `corner_max_wl` means that choosing the partitions that maximise the wirelength 
of connections between the macros to force them to the corners. Vice versa for `corner_min_wl`.

Macros will be placed with $max(halo * 2, channel)$ spacing between macros, and between
macros and the fence/die boundary. If no solutions are found, try reducing the
channel/halo.

# OPTIONS

`-halo`:  Horizontal and vertical halo around macros (microns).

`-channel`:  Horizontal and vertical channel width between macros (microns).

`-fence_region`:  Restrict macro placements to a region (microns). Defaults to the core area.

`-snap_layer_number`:  Snap macro origins to this routing layer track.

`-style`:  Placement style, to choose either `corner_max_wl` or `corner_min_wl`. The default value is `corner_max_wl`.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
