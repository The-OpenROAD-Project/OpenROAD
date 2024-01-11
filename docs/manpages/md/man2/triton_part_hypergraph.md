---
title: triton_part_hypergraph(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/11
---

# NAME

triton_part_hypergraph - triton part hypergraph

# SYNOPSIS

triton_part_hypergraph
    -hypergraph_file hypergraph_file  
    -num_parts num_parts  
    -balance_constraint balance_constraint 
    [-base_balance base_balance]
    [-seed seed] 
    [-vertex_dimension vertex_dimension] 
    [-hyperedge_dimension hyperedge_dimension] 
    [-placement_dimension placement_dimension] 
    [-fixed_file fixed_file] 
    [-community_file community_file] 
    [-group_file group_file] 
    [-placement_file placement_file] 
    [-e_wt_factors e_wt_factors] 
    [-v_wt_factors <v_wt_factors>] 
    [-placement_wt_factors <placement_wt_factors>]
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

This command performs hypergraph netlist partitioning.

# OPTIONS

`-num_parts`:  Number of partitions. The default value is `2`, and the allowed values are integers `[0, MAX_INT]`.

`-balance_constraint`:  Allowed imbalance between blocks. The default value is `1.0`, and the allowed values are floats.

`-base_balance`:  Tcl list of baseline imbalance between partitions. The default value is `{1.0}`, the allowed values are floats that sum up to `1.0`.

`-seed`:  Random seed. The default value is `0`, and the allowed values are integers `[-MAX_INT, MAX_INT]`.

`-vertex_dimension`:  Number of vertices in the hypergraph. The default value is `1`, and the allowed values are integers `[0, MAX_INT]`.

`-hyperedge_dimension`:  Number of hyperedges in hypergraph. The default value is `1`, and the allowed values are integers `[0, MAX_INT]`.

`-placement_dimension`:  Number of dimensions for canvas, if placement information is provided. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`.

`-hypergraph_file`:  Path to hypergraph file.

`-fixed_file`:  Path to fixed vertices constraint file.

`-community_file`:  Path to `community` attributes file to guide the partitioning process.

`-group_file`:  Path to `stay together` attributes file.

`-placement_file`:  Placement information file, each line corresponds to a group fixed vertices, community and placement attributes following the [hMETIS](https://course.ece.cmu.edu/~ee760/760docs/hMetisManual.pdf) format.

`-e_wt_factors`:  Hyperedge weight factor.

`-v_wt_factors`:  Vertex weight factors.

`-placement_wt_factors`:  Placement weight factors.

`-thr_coarsen_hyperedge_size_skip`:  Threshold for ignoring large hyperedge (default 200, integer).

`-thr_coarsen_vertices`:  Number of vertices of coarsest hypergraph (default 10, integer).

`-thr_coarsen_hyperedges`:  Number of vertices of coarsest hypergraph (default 50, integer).

`-coarsening_ratio`:  Coarsening ratio of two adjacent hypergraphs (default 1.6, float).

`-max_coarsen_iters`:  Number of iterations (default 30, integer).

`-adj_diff_ratio`:  Minimum difference of two adjacent hypergraphs (default 0.0001, float).

`-min_num_vertices_each_part`:  Minimum number of vertices in each partition (default 4, integer).

`-num_initial_solutions`:  Number of initial solutions (default 50, integer).

`-num_best_initial_solutions`:  Number of top initial solutions to filter out (default 10, integer).

`-refiner_iters`:  Refinement iterations (default 10, integer).

`-max_moves`:  The allowed moves for each Fiduccia-Mattheyes (FM) algorithm pass or greedy refinement (default 60, integer).

`-early_stop_ratio`:  Describes the ratio $e$ where if the $n_{moved vertices} > n_{vertices} * e$, the tool exits the current FM pass. The intention behind this is that most of the gains are achieved by the first few FM moves. (default 0.5, float).

`-total_corking_passes`:  Maximum level of traversing the buckets to solve the "corking effect" (default 25, integer).

`-v_cycle_flag`:  Disables v-cycle is used to refine partitions (default true, bool).

`-max_num_vcycle`:  Maximum number of `vcycles` (default 1, integer).

`-num_coarsen_solutions`:  Number of coarsening solutions with different randoms seed (default 3, integer).

`-num_vertices_threshold_ilp`:  Describes threshold $t$, the number of vertices used for integer linear programming (ILP) partitioning. if $n_{vertices} > t$, do not use ILP-based partitioning.(default 50, integer).

`-global_net_threshold`:  If the net is larger than this, it will be ignored by TritonPart (default 1000, integer).

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
