---
title: repair_clock_nets(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

repair_clock_nets - repair clock nets

# SYNOPSIS

repair_clock_nets 
    [-max_wire_length max_wire_length]


# DESCRIPTION

The `clock_tree_synthesis` command inserts a clock tree in the design
but may leave a long wire from the clock input pin to the clock tree
root buffer.

The `repair_clock_nets` command inserts buffers in the
wire from the clock input pin to the clock root buffer.

# OPTIONS

`-max_wire_length`:  Maximum length of wires (in microns), defaults to a value that minimizes the wire delay for the wire RC values specified by `set_wire_rc`.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
