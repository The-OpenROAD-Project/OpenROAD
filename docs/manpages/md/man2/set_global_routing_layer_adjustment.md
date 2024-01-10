---
title: set_global_routing_layer_adjustment(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

set_global_routing_layer_adjustment - set global routing layer adjustment

# SYNOPSIS

set_global_routing_layer_adjustment layer adjustment


# DESCRIPTION

The `set_global_routing_layer_adjustment` command sets routing resource
adjustments in the routing layers of the design.  Such adjustments reduce the number of
routing tracks that the global router assumes to exist. This promotes the spreading of routing
and reduces peak congestion, to reduce challenges for detailed routing.

You can set adjustment for a
specific layer, e.g., `set_global_routing_layer_adjustment Metal4 0.5` reduces
the routing resources of routing layer `Metal4` by 50%.  You can also set adjustment
for all layers at once using `*`, e.g., `set_global_routing_layer_adjustment * 0.3` reduces the routing resources of all routing layers by 30%.  And, you can
also set resource adjustment for a layer range, e.g.: `set_global_routing_layer_adjustment
Metal4-Metal8 0.3` reduces the routing resources of routing layers  `Metal4`,
`Metal5`, `Metal6`, `Metal7` and `Metal8` by 30%.

# OPTIONS

This command has no switches.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
