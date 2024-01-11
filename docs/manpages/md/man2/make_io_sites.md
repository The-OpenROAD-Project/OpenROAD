---
title: make_io_sites(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/11
---

# NAME

make_io_sites - make io sites

# SYNOPSIS

make_io_sites 
    -horizontal_site site
    -vertical_site site
    -corner_site site
    -offset offset
    [-rotation_horizontal rotation]
    [-rotation_vertical rotation]
    [-rotation_corner rotation]
    [-ring_index index]


# DESCRIPTION

This command defines an IO site for the pads to be placed into.

Example usage:

```
make_io_sites -horizontal_site IOSITE_H -vertical_site IOSITE_V -corner_site IOSITE_C -offset 35
make_io_sites -horizontal_site IOSITE_H -vertical_site IOSITE_V -corner_site IOSITE_C -offset 35 -rotation_horizontal R180
```

# OPTIONS

`-horizontal_site`:  Name of the site for the horizontal pads (east and west).

`-vertical_site`:  Name of the site for the vertical pads (north and south).

`-corner_site`:  Name of the site for the corner cells.

`-offset`:  Offset from the die edge to place the rows.

`-rotation_horizontal`:  Rotation to apply to the horizontal sites to ensure pads are placed correctly. The default value is `R0`.

`-rotation_vertical`:  Rotation to apply to the vertical sites to ensure pads are placed correctly. The default value is `R0`.

`-rotation_corner`:  Rotation to apply to the corner sites to ensure pads are placed correctly. The default value is `R0`.

`-ring_index`:  Used to specify the index of the ring in case of multiple rings.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
