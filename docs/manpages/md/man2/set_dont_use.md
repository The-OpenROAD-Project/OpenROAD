---
title: set_dont_use(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

set_dont_use - set dont use

# SYNOPSIS

set_dont_use lib_cells # unset_dont_use lib_cells


# DESCRIPTION

The `set_dont_use` command removes library cells from consideration by
the `resizer` engine and the `CTS` engine. `lib_cells` is a list of cells returned by `get_lib_cells`
or a list of cell names (`wildcards` allowed). For example, `DLY*` says do
not use cells with names that begin with `DLY` in all libraries.

# OPTIONS

This command has no switches.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
