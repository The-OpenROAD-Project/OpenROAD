---
title: set_dft_config(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/09
---

# NAME

set_dft_config - set dft config

# SYNOPSIS

set_dft_config 
    [-max_length <int>]
    [-clock_mixing <string>]


# DESCRIPTION

The command `set_dft_config` sets the DFT configuration variables.

# OPTIONS

`-max_length`:  The maxinum number of bits that can be in each scan chain.

`-clock_mixing`:  How architect will mix the scan flops based on the clock driver. `no_mix`: Creates scan chains with only one type of clock and edge. This may create unbalanced chains. `clock_mix`: Craetes scan chains mixing clocks and edges. Falling edge flops are going to be stitched before rising edge.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
