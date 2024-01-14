---
title: rdl_route(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

rdl_route - rdl route

# SYNOPSIS

rdl_route 
    -layer layer
    [-bump_via access_via]
    [-pad_via access_via]
    [-width width]
    [-spacing spacing]
    [-turn_penalty penalty]
    [-allow45]
    nets


# DESCRIPTION

To route the RDL for the bump arrays.

# OPTIONS

`-layer`:  Layer to route on.

`-bump_via`:  Via to use to to connect the bump to the routing layer.

`-pad_via`:  Via to use to to connect the pad cell to the routing layer.

`-width`:  Width of the routing. Defaults to minimum width for each respective layer.

`-spacing`:  Spacing of the routing. Defaults to minimum spacing for each respective layer.

`-turn_penalty`:  Scaling factor to apply to discurage turning to allow for straighter routes. The default value is `2.0`, and the allowed values are floats.

`-allow45`:  Specifies that 45 degree routing is permitted.

`nets`:  Nets to route.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
