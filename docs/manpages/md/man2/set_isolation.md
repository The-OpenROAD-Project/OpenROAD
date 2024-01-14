---
title: set_isolation(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

set_isolation - set isolation

# SYNOPSIS

set_isolation
    [-domain domain]
    [-applies_to applies_to]
    [-clamp_value clamp_value]
    [-isolation_signal isolation_signal]
    [-isolation_sense isolation_sense]
    [-location location]
    [-update]
    name


# DESCRIPTION

This command creates or update isolation strategy.

# OPTIONS

`-domain`:  Power domain

`-applies_to`:  Restricts the strategy to apply one of these (`inputs`, `outputs`, `both`).

`-clamp_value`:  Value the isolation can drive (`0`, `1`).

`-isolation_signal`:  The control signal for this strategy.

`-isolation_sense`:  The active level of isolation control signal.

`-location`:  Domain in which isolation cells are placed (`parent`, `self`, `fanout`).

`-update`:  Only available if using existing strategy, will error if the strategy doesn't exist.

`name`:  Isolation strategy name.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
