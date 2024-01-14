---
title: buffer_ports(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

buffer_ports - buffer ports

# SYNOPSIS

buffer_ports 
    [-inputs] 
    [-outputs] 
    [-max_utilization util]


# DESCRIPTION

The `buffer_ports -inputs` command adds a buffer between the input and its
loads.  The `buffer_ports -outputs` adds a buffer between the port driver
and the output port. Inserting buffers on input and output ports makes
the block input capacitances and output drives independent of the block
internals.

# OPTIONS

`-inputs, -outputs`:  Insert a buffer between the input and load, output and load respectively. The default behavior is `-inputs` and `-outputs` set if neither is specified.

`-max_utilization`:  Defines the percentage of core area used.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
