---
title: add_ruler(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

add_ruler - add ruler

# SYNOPSIS

gui::add_ruler 
       x0 y0 x1 y1
       [label]
       [name]
       [euclidian]


# DESCRIPTION

To add a ruler to the layout:

1. either press ``k`` and use the mouse to place it visually.
To disable snapping for the ruler when adding, hold the ``Ctrl`` key, and to allow non-horizontal or vertical snapping when completing the ruler hold the ``Shift`` key.

2. or use the command:

Returns: name of the newly created ruler.

# OPTIONS

`x0, y0`:  first end point of the ruler in microns.

`x1, y1`:  second end point of the ruler in microns.

`label`:  text label for the ruler.

`name`:  name of the ruler.

`euclidian`:  ``1`` for euclidian ruler, and ``0`` for regular ruler.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
