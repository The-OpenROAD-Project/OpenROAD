---
title: set_routing_alpha(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

set_routing_alpha - set routing alpha

# SYNOPSIS

set_routing_alpha 
    [-net net_name] 
    [-min_fanout fanout]
    [-min_hpwl hpwl]
    alpha


# DESCRIPTION

This command sets routing alphas for a given net `net_name`.

By default the global router uses heuristic rectilinear Steiner minimum
trees (RSMTs) as an initial basis to construct route guides. An RSMT
tries to minimize the total wirelength needed to connect a given set
of pins.  The Prim-Dijkstra heuristic is an alternative net topology
algorithm that supports a trade-off between total wirelength and maximum
path depth from the net driver to its loads. The `set_routing_alpha`
command enables the Prim/Dijkstra algorithm and sets the alpha parameter
used to trade-off wirelength and path depth.  Alpha is between 0.0
and 1.0. When alpha is 0.0 the net topology minimizes total wirelength
(i.e. capacitance).  When alpha is 1.0 it minimizes longest path between
the driver and loads (i.e., maximum resistance).  Typical values are
0.4-0.8. You can call it multiple times for different nets.

Example: `set_routing_alpha -net clk 0.3` sets the alpha value of 0.3 for net *clk*.

# OPTIONS

`-net`:  Net name.

`-min_fanout`:  Set the minimum number for fanout.

`-min_hpwl`:  Set the minimum half-perimetere wirelength (microns).

`alpha`:  Float between 0 and 1 describing the trade-off between wirelength and path depth.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
