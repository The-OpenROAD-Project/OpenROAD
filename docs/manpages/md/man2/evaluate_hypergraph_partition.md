---
title: evaluate_hypergraph_partition(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

evaluate_hypergraph_partition - evaluate hypergraph partition

# SYNOPSIS

evaluate_hypergraph_solution
  -num_parts num_parts
  -balance_constraint balance_constraint
  -hypergraph_file hypergraph_file
  -solution_file solution_file
  [-base_balance base_balance]
  [-vertex_dimension vertex_dimension]
  [-hyperedge_dimension hyperedge_dimension]
  [-fixed_file fixed_file]
  [-group_file group_file]
  [-e_wt_factors e_wt_factors]
  [-v_wt_factors v_wt_factors] 


# DESCRIPTION

This command evaluates hypergraph partition.

# OPTIONS

`-num_parts`:  Number of partitions. The default value is `2`, and the allowed values are integers `[0, MAX_INT]`.

`-balance_constraint`:  Allowed imbalance between blocks. The default value is `1.0`, and the allowed values are floats.

`-vertex_dimension`:  Number of vertices in the hypergraph. The default value is `1`, and the allowed values are integers `[0, MAX_INT]`.

`-hyperedge_dimension`:  Number of hyperedges in hypergraph. The default value is `1`, and the allowed values are integers `[0, MAX_INT]`.

`-hypergraph_file`:  Path to hypergraph file.

`-solution_file`:  Path to solution file.

`-base_balance`:  Tcl list of baseline imbalance between partitions. The default value is `{1.0}`, the allowed values are floats that sum up to `1.0`.

`-fixed_file`:  Path to fixed vertices constraint file.

`-group_file`:  Path to `stay together` attributes file.

`-e_wt_factors`:  Hyperedge weight factor.

`-v_wt_factors`:  Vertex weight factor.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
