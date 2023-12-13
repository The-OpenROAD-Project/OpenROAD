---
title: set_pin_thick_multiplier(2)
author: Jack Luar (TODO@TODO.com)
date: 23/12/13
---

# NAME

set_pin_thick_multiplier - set pin thick multiplier

# SYNOPSIS

set_pin_thick_multiplier 
    [-hor_multiplier h_mult]
    [-ver_multiplier v_mult]


# DESCRIPTION

The `set_pin_thick_multiplier` command defines a multiplier for the thickness of all
vertical and horizontal pins.

# OPTIONS

`-temperature`:  Temperature parameter. The default value is `1.0`, and the allowed values are floats `[0, MAX_FLOAT]`.

`-max_iterations`:  The maximum number of iterations. The default value is `2000`, and the allowed values are integers `[0, MAX_INT]`.

`-perturb_per_iter`:  The number of perturbations per iteration. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`.

`-alpha`:  The temperature decay factor. The default value is `0.985`, and the allowed values are floats `(0, 1]`.

# ARGUMENTS

# EXAMPLES

# ENVIRONMENT

# FILES

# SEE ALSO

# HISTORY

# BUGS

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
