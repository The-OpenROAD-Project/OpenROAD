---
title: place_pad(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

place_pad - place pad

# SYNOPSIS

place_pad 
    -row row_name
    -location offset
    -mirror
    [-master master]
    name


# DESCRIPTION

To place a pad into the pad ring.

Example usage:

```
place_pad -row IO_SOUTH -location 280.0 {u_clk.u_in}
place_pad -row IO_SOUTH -location 360.0 -mirror {u_reset.u_in}
place_pad -master sky130_fd_io__top_ground_hvc_wpad -row IO_SOUTH -location 439.5 {u_vzz_0}
place_pad -master sky130_fd_io__top_power_hvc_wpad -row IO_SOUTH -location 517.5 {u_v18_0}
```

# OPTIONS

`-row`:  Name of the row to place the pad into, examples include: `IO_NORTH`, `IO_SOUTH`, `IO_WEST`, `IO_EAST`, `IO_NORTH_0`, `IO_NORTH_1`.

`-location`:  Offset from the bottom left chip edge to place the pad at.

`-mirror`:  Specifies if the pad should be mirrored.

`-master`:  Name of the instance master if the instance needs to be created.

`name`:  Name of the instance.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
