---
title: set_routing_layers(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

set_routing_layers - set routing layers

# SYNOPSIS

set_routing_layers 
    [-signal min-max]
    [-clock min-max]


# DESCRIPTION

This command sets the minimum and maximum routing layers for signal and clock nets.
Example: `set_routing_layers -signal Metal2-Metal10 -clock Metal6-Metal9`

# OPTIONS

`-signal`:  Set the min and max routing signal layer (names) in this format "%s-%s".

`-clock`:  Set the min and max routing clock layer (names) in this format "%s-%s".

`Argument Name`:  Description

`extension`:  Number of `GCells` added to the blockage boundaries from macros. The default `GCell` size is 15 `M3` pitches.

`offset`:  Pin offset in microns (must be a positive integer).

`layer`:  Integer for the layer number (e.g. for M1 you would use 1).

`adjustment`:  Float indicating the percentage reduction of each edge in the specified layer.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
