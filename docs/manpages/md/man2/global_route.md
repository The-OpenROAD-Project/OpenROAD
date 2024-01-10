---
title: global_route(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

global_route - global route

# SYNOPSIS

global_route 
    [-guide_file out_file]
    [-congestion_iterations iterations]
    [-congestion_report_file file_name]
    [-congestion_report_iter_step steps]
    [-grid_origin {x y}]
    [-critical_nets_percentage percent]
    [-allow_congestion]
    [-verbose]
    [-start_incremental]
    [-end_incremental]


# DESCRIPTION

This command performs global routing with the option to use a `guide_file`.
You may also choose to use incremental global routing using `-start_incremental`.

# OPTIONS

`-guide_file`:  Set the output guides file name (e.g., `route.guide`).

`-congestion_iterations`:  Set the number of iterations made to remove the overflow of the routing. The default value is `50`, and the allowed values are integers `[0, MAX_INT]`.

`-congestion_report_file`:  Set the file name to save the congestion report. The file generated can be read by the DRC viewer in the GUI (e.g., `report_file.rpt`).

`-congestion_report_iter_step`:  Set the number of iterations to report. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`.

`-grid_origin`:  Set the (x, y) origin of the routing grid in DBU. For example, `-grid_origin {1 1}` corresponds to the die (0, 0) + 1 DBU in each x--, y- direction.

`-critical_nets_percentage`:  Set the percentage of nets with the worst slack value that are considered timing critical, having preference over other nets during congestion iterations (e.g. `-critical_nets_percentage 30`). The default value is `0`, and the allowed values are integers `[0, MAX_INT]`.

`-allow_congestion`:  Allow global routing results to be generated with remaining congestion. The default is false.

`-verbose`:  This flag enables the full reporting of the global routing.

`-start_incremental`:  This flag initializes the GRT listener to get the net modified. The default is false.

`-end_incremental`:  This flag run incremental GRT with the nets modified. The default is false.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
