---
title: place_io_terminals(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

place_io_terminals - place io terminals

# SYNOPSIS

place_io_terminals 
    inst_pins


# DESCRIPTION

In the case where the bond pads are integrated into the padcell, the IO terminals need to be placed.
This command place terminals on the padring.

Example usage: 
```
place_io_terminals u_*/PAD
place_io_terminals u_*/VDD
```

# OPTIONS

`inst_pins`:  Instance pins to place the terminals on.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
