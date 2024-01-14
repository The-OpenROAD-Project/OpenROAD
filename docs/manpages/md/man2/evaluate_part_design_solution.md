---
title: evaluate_part_design_solution(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

evaluate_part_design_solution - evaluate part design solution

# SYNOPSIS

evaluate_part_design_solution
    [-num_parts num_parts]
    [-balance_constraint balance_constraint]
    [-base_balance base_balance]
    [-timing_aware_flag timing_aware_flag]
    [-top_n top_n]
    [-fence_flag fence_flag]
    [-fence_lx fence_lx]
    [-fence_ly fence_ly]
    [-fence_ux fence_ux]
    [-fence_uy fence_uy]
    [-fixed_file fixed_file]
    [-community_file community_file]
    [-group_file group_file]
    [-hypergraph_file hypergraph_file]
    [-hypergraph_int_weight_file hypergraph_int_weight_file]
    [-solution_file solution_file]
    [-net_timing_factor net_timing_factor]
    [-path_timing_factor path_timing_factor]
    [-path_snaking_factor path_snaking_factor]
    [-timing_exp_factor timing_exp_factor]
    [-extra_delay extra_delay]
    [-guardband_flag guardband_flag]
    [-e_wt_factors e_wt_factors]
    [-v_wt_factors v_wt_factors]


# DESCRIPTION

This command evaluates partition design solution.

# OPTIONS

`-num_parts`:  Number of partitions. The default value is `2`, and the allowed values are integers `[0, MAX_INT]`.

`-balance_constraint`:  Allowed imbalance between blocks. The default value is `1.0`, and the allowed values are floats.

`-base_balance`:  Tcl list of baseline imbalance between partitions. The default value is `{1.0}`, the allowed values are floats that sum up to `1.0`.

`-timing_aware_flag`:  Enable timing-driven mode. The default value is `true`, and the allowed values are booleans.

`-top_n`:  Extract the top n critical timing paths. The default value is `1000`, and the allowed values are integers `[0, MAX_INT]`.

`-fence_flag`:  Consider fences in the partitioning. The default value is `false`, and the allowed values are booleans.

`-fence_lx`:  Fence lower left x in microns. The default value is `0.0`, and the allowed values are floats.

`-fence_ly`:  Fence lower left y in microns. The default value is `0.0`, and the allowed values are floats.

`-fence_ux`:  Fence upper right x in microns. The default value is `0.0`, and the allowed values are floats.

`-fence_uy`:  Fence upper right y in microns. The default value is `0.0`, and the allowed values are floats.

`-fixed_file`:  Path to fixed vertices constraint file.

`-community_file`:  Path to `community` attributes file to guide the partitioning process.

`-group_file`:  Path to `stay together` attributes file.

`-hypergraph_file`:  Path to hypergraph file.

`-hypergraph_int_weight_file`:  Path to `hMETIS` format integer weight file.

`-solution_file`:  Path to solution file.

`-net_timing_factor`:  Hyperedge timing weight factor. The default value is `1.0`, and the allowed values are floats.

`-path_timing_factor`:  Cutting critical timing path weight factor. The default value is `1.0`, and the allowed values are floats.

`-path_snaking_factor`:  Snaking a critical path weight factor. The default value is `1.0`, and the allowed values are floats.

`-timing_exp_factor`:  Timing exponential factor for normalized slack. Thedefault value is `1.0`, and the allowed values are floats.

`-extra_delay`:  Extra delay introduced by a cut. The default value is `1e-9`, and the allowed values are floats.

`-guardband_flag`:  Enable timing guardband option. The default value is 1`false`, and the allowed values are booleans.

`-e_wt_factors`:  Hyperedge weight factors.

`-v_wt_factors`:  Vertex weight factors.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
