---
date: 23/12/17
section: 2
title: set\_pin\_thick\_multiplier
---

NAME
====

set\_pin\_thick\_multiplier - set pin thick multiplier

SYNOPSIS
========

set\_pin\_thick\_multiplier \[-hor\_multiplier h\_mult\]
\[-ver\_multiplier v\_mult\]

DESCRIPTION
===========

The `set_pin_thick_multiplier` command defines a multiplier for the
thickness of all vertical and horizontal pins.

OPTIONS
=======

`-temperature`: Temperature parameter. The default value is `1.0`, and
the allowed values are floats `[0, MAX_FLOAT]`.

`-max_iterations`: The maximum number of iterations. The default value
is `2000`, and the allowed values are integers `[0, MAX_INT]`.

`-perturb_per_iter`: The number of perturbations per iteration. The
default value is `0`, and the allowed values are integers
`[0, MAX_INT]`.

`-alpha`: The temperature decay factor. The default value is `0.985`,
and the allowed values are floats `(0, 1]`.

ARGUMENTS
=========

EXAMPLES
========

ENVIRONMENT
===========

FILES
=====

SEE ALSO
========

HISTORY
=======

BUGS
====

COPYRIGHT
=========

Copyright (c) 2024, The Regents of the University of California. All
rights reserved.

AUTHORS
=======

Jack Luar (TODO\@TODO.com).
