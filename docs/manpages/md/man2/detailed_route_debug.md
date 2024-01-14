---
title: detailed_route_debug(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

detailed_route_debug - detailed route debug

# SYNOPSIS

detailed_route_debug 
    [-pa]
    [-ta]
    [-dr]
    [-maze]
    [-net name]
    [-pin name]
    [-worker x y]
    [-iter iter]
    [-pa_markers]
    [-dump_dr]
    [-dump_dir dir]
    [-pa_edge]
    [-pa_commit]
    [-write_net_tracks]


# DESCRIPTION

The following command and arguments are useful when debugging error
messages from `drt` and to understand its behavior.

# OPTIONS

`-pa`:  Enable debug for pin access.

`-ta`:  Enable debug for track assignment.

`-dr`:  Enable debug for detailed routing.

`-maze`:  Enable debug for maze routing.

`-net`:  Enable debug for net name.

`-pin`:  Enable debug for pin name.

`-worker`:  Debugs routes that pass through the point `{x, y}`.

`-iter`:  Specifies the number of debug iterations. The default value is `0`, and the accepted values are integers `[0, MAX_INT`.

`-pa_markers`:  Enable pin access markers.

`-dump_dr`:  Filename for detailed routing dump.

`-dump_dir`:  Directory for detailed routing dump.

`-pa_edge`:  Enable visibility of pin access edges.

`-pa_commit`:  Enable visibility of pin access commits.

`-write_net_tracks`:  Enable writing of net track assigments.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
