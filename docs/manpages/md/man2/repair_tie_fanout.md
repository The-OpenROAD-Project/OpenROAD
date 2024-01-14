---
title: repair_tie_fanout(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

repair_tie_fanout - repair tie fanout

# SYNOPSIS

repair_tie_fanout 
    [-separation dist]
    [-verbose]
    lib_port


# DESCRIPTION

The `repair_tie_fanout` command connects each tie high/low load to a copy
of the tie high/low cell.

# OPTIONS

`-separation`:  Tie high/low insts are separated from the load by this value (Liberty units, usually microns).

`-verbose`:  Enable verbose logging of repair progress.

`lib_port`:  Tie high/low port, which can be a library/cell/port name or object returned by `get_lib_pins`.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
