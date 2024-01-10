---
title: select_at(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

select_at - select at

# SYNOPSIS

gui::select_at x y
gui::select_at x y append
gui::select_at x0 y0 x1 y1
gui::select_at x0 y0 x1 y1 append


# DESCRIPTION

To add items at a specific point or in an area:

# OPTIONS

`x, y`:  point in the layout area in microns.

`x0, y0`:  first corner of the layout area in microns.

`x1, y1`:  second corner of the layout area in microns.

`append`:  if ``true`` (the default value) append the new selections to the current selection list, else replace the selection list with the new selections.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
