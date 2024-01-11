---
title: place_pads(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/11
---

# NAME

place_pads - place pads

# SYNOPSIS

place_pad 
    -row row_name
    -location offset
    -mirror
    [-master master]
    name


# DESCRIPTION

To place a pad into the pad ring.

# OPTIONS

`-row`:  Name of the row to place the pad into, examples include: `IO_NORTH`, `IO_SOUTH`, `IO_WEST`, `IO_EAST`, `IO_NORTH_0`, `IO_NORTH_1`.

`-location`:  Offset from the bottom left chip edge to place the pad at.

`-mirror`:  Specifies if the pad should be mirrored.

`-master`:  Name of the instance master if the instance needs to be created.

`name`:  Name of the instance.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
