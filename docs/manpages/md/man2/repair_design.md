---
title: repair_design(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

repair_design - repair design

# SYNOPSIS

repair_design 
    [-max_wire_length max_length]
    [-slew_margin slew_margin]
    [-cap_margin cap_margin]
    [-max_utilization util]
    [-verbose]


# DESCRIPTION

The `repair_design` command inserts buffers on nets to repair max slew, max
capacitance and max fanout violations, and on long wires to reduce RC delay in
the wire. It also resizes gates to normalize slews.  Use `estimate_parasitics
-placement` before `repair_design` to estimate parasitics considered
during repair. Placement-based parasitics cannot accurately predict
routed parasitics, so a margin can be used to "over-repair" the design
to compensate.

# OPTIONS

`-max_wire_length`:  Maximum length of wires (in microns), defaults to a value that minimizes the wire delay for the wire RC values specified by `set_wire_rc`.

`-slew_margin`:  Add a slew margin. The default value is `0`, the allowed values are integers `[0, 100]`.

`-cap_margin`:  Add a capactitance margin. The default value is `0`, the allowed values are integers `[0, 100]`.

`-max_utilization`:  Defines the percentage of core area used.

`-verbose`:  Enable verbose logging on progress of the repair.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
