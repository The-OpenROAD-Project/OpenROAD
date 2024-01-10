---
title: global_route_debug(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

global_route_debug - global route debug

# SYNOPSIS

global_route_debug 
    [-st]
    [-rst]
    [-tree2D]
    [-tree3D]
    [-saveSttInput file_name]
    [-net net_name]


# DESCRIPTION

The `global_route_debug` command allows you to start a debug mode to view the status of the Steiner Trees.
It also allows you to dump the input positions for the Steiner tree creation of a net.
This must be used before calling the `global_route` command. 
Set the name of the net and the trees that you want to visualize.

# OPTIONS

`-net`:  List of nets to report the wirelength. Use `*` to report the wire length for all nets of the design.

`-file`:  The name of the file for the wirelength report.

`-global_route`:  Report the wire length of the global routing.

`-detailed_route`:  Report the wire length of the detailed routing.

`-verbose`:  This flag enables the full reporting of the layer-wise wirelength information.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
