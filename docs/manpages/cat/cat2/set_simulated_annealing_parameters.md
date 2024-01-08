---
date: 23/12/17
section: 2
title: set\_simulated\_annealing\_parameters
---

NAME
====

set\_simulated\_annealing\_parameters - set simulated annealing
parameters

SYNOPSIS
========

set\_simulated\_annealing \[-temperature temperature\]
\[-max\_iterations iter\] \[-perturb\_per\_iter perturbs\] \[-alpha
alpha\]

DESCRIPTION
===========

The `set_simulated_annealing` command defines the parameters for
simulated annealing pin placement.

OPTIONS
=======

`-pin_name`: The name of a pin of the design.

`-layer`: The routing layer where the pin is placed.

`-location`: The center of the pin (in microns).

`-pin_size`: Tthe width and height of the pin (in microns).

`-force_to_die_boundary`: When this flag is enabled, the pin will be
snapped to the nearest routing track, next to the die boundary.

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
