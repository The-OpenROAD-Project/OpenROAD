---
title: detailed_route(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

detailed_route - detailed route

# SYNOPSIS

detailed_route 
    [-output_maze filename]
    [-output_drc filename]
    [-output_cmap filename]
    [-output_guide_coverage filename]
    [-drc_report_iter_step step]
    [-db_process_node name]
    [-disable_via_gen]
    [-droute_end_iter iter]
    [-via_in_pin_bottom_layer layer]
    [-via_in_pin_top_layer layer]
    [-or_seed seed]
    [-or_k_ k]
    [-bottom_routing_layer layer]
    [-top_routing_layer layer]
    [-verbose level]
    [-distributed]
    [-remote_host rhost]
    [-remote_port rport]
    [-shared_volume vol]
    [-cloud_size sz]
    [-clean_patches]
    [-no_pin_access]
    [-min_access_points count]
    [-save_guide_updates]
    [-repair_pdn_vias layer]


# DESCRIPTION

This command performs detailed routing.

# OPTIONS

`-output_maze`:  Path to output maze log file (e.g. `output_maze.log`).

`-output_drc`:  Path to output DRC report file (e.g. `output_drc.rpt`).

`-output_cmap`:  Path to output congestion map file (e.g. `output.cmap`).

`-output_guide_coverage`:  Path to output guide coverage file (e.g. `sample_coverage.csv`).

`-drc_report_iter_step`:  Report DRC on each iteration which is a multiple of this step. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`.

`-db_process_node`:  Specify the process node.

`-disable_via_gen`:  Option to diable via generation with bottom and top routing layer. The default value is disabled.

`-droute_end_iter`:  Number of detailed routing iterations. The default value is `-1`, and the allowed values are integers `[1, 64]`.

`-via_in_pin_bottom_layer`:  Via-in pin bottom layer name.

`-via_in_pin_top_layer`:  Via-in pin top layer name.

`-or_seed`:  Refer to developer arguments [here](#developer-arguments).

`-or_k`:  Refer to developer arguments [here](#developer-arguments).

`-bottom_routing_layer`:  Bottommost routing layer name.

`-top_routing_layer`:  Topmost routing layer name.

`-verbose`:  Sets verbose mode if the value is greater than 1, else non-verbose mode (must be integer, or error will be triggered.)

`-distributed`:  Refer to distributed arguments [here](#distributed-arguments).

`-clean_patches`:  Clean unneeded patches during detailed routing.

`-no_pin_access`:  Disables pin access for routing.

`-min_access_points`:  Minimum access points for standard cell and macro cell pins.

`-save_guide_updates`:  Flag to save guides updates.

`-repair_pdn_vias`:  This option is used for PDKs where M1 and M2 power rails run in parallel.

# ARGUMENTS

`-or_seed`:  Random seed for the order of nets to reroute. The default value is `-1`, and the allowed values are integers `[0, MAX_INT]`.

`-or_k`:  Number of swaps is given by $k * sizeof(rerouteNets)$. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
