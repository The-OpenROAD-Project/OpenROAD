---
title: triton_part_design(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

triton_part_design - triton part design

# SYNOPSIS

triton_part_design
    [-num_parts num_parts]
    [-balance_constraint balance_constraint]
    [-base_balance base_balance]
    [-seed seed]
    [-timing_aware_flag timing_aware_flag]
    [-top_n top_n]
    [-placement_flag placement_flag]
    [-fence_flag fence_flag]
    [-fence_lx fence_lx]
    [-fence_ly fence_ly]
    [-fence_ux fence_ux]
    [-fence_uy fence_uy]
    [-fixed_file fixed_file]
    [-community_file community_file]
    [-group_file group_file]
    [-solution_file solution_file]
    [-net_timing_factor net_timing_factor]
    [-path_timing_factor path_timing_factor]
    [-path_snaking_factor path_snaking_factor]
    [-timing_exp_factor timing_exp_factor]
    [-extra_delay extra_delay]
    [-guardband_flag guardband_flag]
    [-e_wt_factors e_wt_factors]
    [-v_wt_factors v_wt_factors]
    [-placement_wt_factors placement_wt_factors]
    [-thr_coarsen_hyperedge_size_skip thr_coarsen_hyperedge_size_skip]
    [-thr_coarsen_vertices thr_coarsen_vertices]
    [-thr_coarsen_hyperedges thr_coarsen_hyperedges]
    [-coarsening_ratio coarsening_ratio]
    [-max_coarsen_iters max_coarsen_iters]
    [-adj_diff_ratio adj_diff_ratio]
    [-min_num_vertices_each_part min_num_vertices_each_part]
    [-num_initial_solutions num_initial_solutions]
    [-num_best_initial_solutions num_best_initial_solutions]
    [-refiner_iters refiner_iters]
    [-max_moves max_moves]
    [-early_stop_ratio early_stop_ratio]
    [-total_corking_passes total_corking_passes]
    [-v_cycle_flag v_cycle_flag ]
    [-max_num_vcycle max_num_vcycle]
    [-num_coarsen_solutions num_coarsen_solutions]
    [-num_vertices_threshold_ilp num_vertices_threshold_ilp]
    [-global_net_threshold global_net_threshold]


# DESCRIPTION

This command partitions the design netlist. Note that design must be loaded in memory.

# OPTIONS

`-num_parts`:  Number of partitions. The default value is `2`, and the allowed values are integers `[0, MAX_INT]`.

`-balance_constraint`:  Allowed imbalance between blocks. The default value is `1.0`, and the allowed values are floats.

`-base_balance`:  Tcl list of baseline imbalance between partitions. The default value is `{1.0}`, the allowed values are floats that sum up to `1.0`.

`-seed`:  Random seed. The default value is `1`, and the allowed values are integers `[-MAX_INT, MAX_INT]`.

`-timing_aware_flag`:  Enable timing-driven mode. The default value is `true`, and the allowed values are booleans.

`-top_n`:  Extract the top n critical timing paths. The default value is `1000`, and the allowed values are integers `[0, MAX_INT`.

`-placement_flag`:  Enable placement driven partitioning. The default value is `false`, and the allowed values are booleans.

`-fence_flag`:  Consider fences in the partitioning. The default value is `false`, and the allowed values are booleans.

`-fence_lx`:  Fence lower left x in microns. The default value is `0.0`, and the allowed values are floats.

`-fence_ly`:  Fence lower left y in microns. The default value is `0.0`, and the allowed values are floats.

`-fence_ux`:  Fence upper right x in microns. The default value is `0.0`, and the allowed values are floats.

`-fence_uy`:  Fence upper right y in microns. The default value is `0.0`, and the allowed values are floats.

`-fixed_file`:  Path to fixed vertices constraint file

`-community_file`:  Path to `community` attributes file to guide the partitioning process.

`-group_file`:  Path to `stay together` attributes file.

`-solution_file`:  Path to solution file.

`-net_timing_factor`:  Hyperedge timing weight factor (default 1.0, float).

`-path_timing_factor`:  Cutting critical timing path weight factor (default 1.0, float).

`-path_snaking_factor`:  Snaking a critical path weight factor (default 1.0, float).

`-timing_exp_factor`:  Timing exponential factor for normalized slack (default 1.0, float).

`-extra_delay`:  Extra delay introduced by a cut (default 1e-9, float).

`-guardband_flag`:  Enable timing guardband option (default false, bool).

`-e_wt_factors`:  Hyperedge weight factor.

`-v_wt_factors`:  Vertex weight factor.

`-placement_wt_factors`:  Placement weight factor.

`-thr_coarsen_hyperedge_size_skip`:  Threshold for ignoring large hyperedge. The default value is `1000`, and the allowed values are integers `[0, MAX_INT]`.

`-thr_coarsen_vertices`:  Number of vertices of coarsest hypergraph. The default value is `10`, and the allowed values are integers `[0, MAX_INT]`.

`-thr_coarsen_hyperedges`:  Number of vertices of coarsest hypergraph. The default value is `50`, and the allowed values are integers `[0, MAX_INT]`.

`-coarsening_ratio`:  Coarsening ratio of two adjacent hypergraphs. The default value is `1.5`, and the allowed values are floats.

`-max_coarsen_iters`:  Number of iterations. The default value is `30`, and the allowed values are integers `[0, MAX_INT]`.

`-adj_diff_ratio`:  Minimum ratio difference of two adjacent hypergraphs. The default value is `0.0001`, and the allowed values are floats.

`-min_num_vertices_each_part`:  Minimum number of vertices in each partition. The default value is `4`, and the allowed values are integers `[0, MAX_INT]`.

`-num_initial_solutions`:  Number of initial solutions. The default value is `100`, and the allowed values are integers `[0, MAX_INT]`.

`-num_best_initial_solutions`:  Number of top initial solutions to filter out. The default value is `10`, and the allowed values are integers `[0, MAX_INT]`.

`-refiner_iters`:  Refinement iterations. The default value is `10`, and the allowed values are integers `[0, MAX_INT]`.

`-max_moves`:  The allowed moves for each Fiduccia-Mattheyes (FM) algorithm pass or greedy refinement. The default value is `100`, and the allowed values are integers `[0, MAX_INT]`.

`-early_stop_ratio`:  Describes the ratio $e$ where if the $n_{moved vertices} > n_{vertices} * e$, the tool exists the current FM pass. The intention behind this being that most of the gains are achieved by the first few FM moves. The default value is `0.5`, and the allowed values are floats.

`-total_corking_passes`:  Maximum level of traversing the buckets to solve the "corking effect". The default value is `25`, and the allowed values are integers `[0, MAX_INT]`.

`-v_cycle_flag`:  Disables v-cycle is used to refine partitions. The default value is `true`, and the allowed values are booleans.

`-max_num_vcycle`:  Maximum number of vcycles. The default value is `1`, and the allowed values are integers `[0, MAX_INT]`.

`-num_coarsen_solutions`:  Number of coarsening solutions with different randoms seed. The default value is `4`, and the allowed values are integers `[0, MAX_INT]`.

`-num_vertices_threshold_ilp`:  Describes threshold $t$, the number of vertices used for integer linear programming (ILP) partitioning. if $n_{vertices} > t$, do not use ILP-based partitioning. The default value is `50`, and the allowed values are integers `[0, MAX_INT]`.

`-global_net_threshold`:  If the net is larger than this, it will be ignored by TritonPart. The default value is `1000`, and the allowed values are integers `[0, MAX_INT]`.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
