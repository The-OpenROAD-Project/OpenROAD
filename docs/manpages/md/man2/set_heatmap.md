---
title: set_heatmap(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

set_heatmap - set heatmap

# SYNOPSIS

gui::set_heatmap 
       name
       [option]
       [value]


# DESCRIPTION

To control the settings in the heat maps:

The currently availble heat maps are:
- ``Power``
- ``Routing``
- ``Placement``
- ``IRDrop``

These options can also be modified in the GUI by double-clicking the underlined display control for the heat map.

# OPTIONS

`name`:  is the name of the heatmap.

`option`:  is the name of the option to modify. If option is ``rebuild`` the map will be destroyed and rebuilt.

`value`:  is the new value for the specified option. This is not used when rebuilding map.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
