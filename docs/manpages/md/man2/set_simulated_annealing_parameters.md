---
title: set_simulated_annealing_parameters(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/09
---

# NAME

set_simulated_annealing_parameters - set simulated annealing parameters

# SYNOPSIS

set_simulated_annealing
    [-temperature temperature]
    [-max_iterations iter]
    [-perturb_per_iter perturbs]
    [-alpha alpha]


# DESCRIPTION

The `set_simulated_annealing` command defines the parameters for simulated annealing pin placement.

# OPTIONS

`-temperature`:  Temperature parameter. The default value is `1.0`, and the allowed values are floats `[0, MAX_FLOAT]`.

`-max_iterations`:  The maximum number of iterations. The default value is `2000`, and the allowed values are integers `[0, MAX_INT]`.

`-perturb_per_iter`:  The number of perturbations per iteration. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`.

`-alpha`:  The temperature decay factor. The default value is `0.985`, and the allowed values are floats `(0, 1]`.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
