---
title: estimate_parasitics(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

estimate_parasitics - estimate parasitics

# SYNOPSIS

estimate_parasitics
    -placement|-global_routing


# DESCRIPTION

Estimate RC parasitics based on placed component pin locations. If there are
no component locations, then no parasitics are added. The resistance and capacitance
values are per distance unit of a routing wire. Use the `set_units` command to check
units or `set_cmd_units` to change units. The goal is to represent "average"
routing layer resistance and capacitance. If the set_wire_rc command is not
called before resizing, then the default_wireload model specified in the first
Liberty file read or with the SDC set_wire_load command is used to make parasitics.

After the `global_route` command has been called, the global routing topology
and layers can be used to estimate parasitics  with the `-global_routing`
flag.

# OPTIONS

`-placement or -global_routing`:  Either of these flags must be set. Parasitics are estimated based after placement stage versus after global routing stage.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
