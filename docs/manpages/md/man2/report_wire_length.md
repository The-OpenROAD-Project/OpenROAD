---
title: report_wire_length(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

report_wire_length - report wire length

# SYNOPSIS

report_wire_length 
    [-net net_list]
    [-file file]
    [-global_route]
    [-detailed_route]
    [-verbose]


# DESCRIPTION

The `report_wire_length` command reports the wire length of the nets. Use the `-global_route`
and the `-detailed_route` flags to report the wire length from global and detailed routing,
respectively. If none of these flags are used, the tool will identify the state of the design
and report the wire length accordingly.

Example: `report_wire_length -net {clk net60} -global_route -detailed_route -verbose -file out.csv`

# OPTIONS

`net_names`:  Tcl list of set of nets (e.g. `{net1, net2}`).

`-show_pin_locations`:  Draw circles for the pin positions on the routing grid.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
