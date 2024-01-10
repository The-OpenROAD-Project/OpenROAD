---
title: repair_antennas(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

repair_antennas - repair antennas

# SYNOPSIS

repair_antennas 
    [diode_cell]
    [-iterations iterations]
    [-ratio_margin margin]


# DESCRIPTION

The `repair_antennas` command checks the global routing for antenna
violations and repairs the violations by inserting diodes near the
gates of the violating nets.  By default the command runs only one
iteration to repair antennas. Filler instances added by the
`filler_placement` command should NOT be in the database when
`repair_antennas` is called. 

See LEF/DEF 5.8 Language Reference, Appendix C, "Calculating and
Fixing Process Antenna Violations" for a [description](coriolis.lip6.fr/doc/lefdef/lefdefref/lefdefref.pdf) 
of antenna violations.

If no `diode_cell` argument is specified the LEF cell with class CORE, ANTENNACELL will be used.
If any repairs are made the filler instances are remove and must be
placed with the `filler_placement` command.

If the LEF technology layer `ANTENNADIFFSIDEAREARATIO` properties are constant
instead of PWL, inserting diodes will not improve the antenna ratios, 
and thus, no
diodes are inserted. The following warning message will be reported:

```
[WARNING GRT-0243] Unable to repair antennas on net with diodes.
```

# OPTIONS

`diode_cell`:  Diode cell to fix antenna violations.

`-iterations`:  Number of iterations. The default value is `1`, and the allowed values are integers `[0, MAX_INT]`.

`-ratio_margin`:  Add a margin to the antenna ratios. The default value is `0`, and the allowed values are integers `[0, 100]`.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
