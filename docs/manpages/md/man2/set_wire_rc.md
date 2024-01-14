---
title: set_wire_rc(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

set_wire_rc - set wire rc

# SYNOPSIS

set_wire_rc 
    [-clock] 
    [-signal]
    [-layer layer_name]

or 
set_wire_rc
    [-resistance res]
    [-capacitance cap]


# DESCRIPTION

The `set_wire_rc` command sets the resistance and capacitance used to estimate
delay of routing wires.  Separate values can be specified for clock and data
nets with the `-signal` and `-clock` flags. Without either `-signal` or
`-clock` the resistance and capacitance for clocks and data nets are set.

# OPTIONS

`-clock`:  Enable setting of RC for clock nets.

`-signal`:  Enable setting of RC for signal nets.

`-layer`:  Use the LEF technology resistance and area/edge capacitance values for the layer. This is used for a default width wire on the layer.

`-resistance`:  Resistance per unit length, units are from the first Liberty file read, usually in the form of $\frac{resistanceUnit}{distanceUnit}$. Usually kΩ/µm.

`-capacitance`:  Capacitance per unit length, units are from the first Liberty file read, usually in the form of $\frac{capacitanceUnit}{distanceUnit}$. Usually pF/µm.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
