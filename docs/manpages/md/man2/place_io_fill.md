---
title: place_io_fill(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/11
---

# NAME

place_io_fill - place io fill

# SYNOPSIS

place_io_fill 
    -row row_name
    [-permit_overlaps masters]
    masters


# DESCRIPTION

To place the IO filler cells.

Example usage: 

```
place_io_fill -row IO_NORTH s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
place_io_fill -row IO_SOUTH s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
place_io_fill -row IO_WEST s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
place_io_fill -row IO_EAST s8iom0s8_com_bus_slice_10um s8iom0s8_com_bus_slice_5um s8iom0s8_com_bus_slice_1um
```

# OPTIONS

`-row`:  Name of the row to place the pad into, examples include: `IO_NORTH`, `IO_SOUTH`, `IO_WEST`, `IO_EAST`, `IO_NORTH_0`, `IO_NORTH_1`.

`-permit_overlaps`:  Names of the masters for the IO filler cells that allow for overlapping.

`masters`:  Names of the masters for the IO filler cells.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
