---
title: set_placement_padding(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

set_placement_padding - set placement padding

# SYNOPSIS

set_placement_padding   
    -global|-masters masters|-instances insts
    [-right site_count]
    [-left site_count]


# DESCRIPTION

The `set_placement_padding` command sets left and right padding in multiples
of the row site width. Use the `set_placement_padding` command before
legalizing placement to leave room for routing. Use the `-global` flag
for padding that applies to all instances. Use  `-instances`
for instance-specific padding.  The instances `insts` can be a list of instance
names, or an instance object returned by the SDC `get_cells` command. To
specify padding for all instances of a common master, use the `-filter`
"ref_name == <name>" option to `get_cells`.

# OPTIONS

`-global`:  Set padding globally using `left` and `right` values.

`-masters`:   Set padding only for these masters using `left` and `right` values.

`-instances`:  For `-instances`, you will set padding only for these insts using `left` and `right` values.

`-left`:  Left padding (in site count).

`-right`:  Right padding (in site count).

`instances`:  Set padding for these list of instances. Not to be confused with the `-instances` switch above.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
