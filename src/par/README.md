# TritonPart : An Open-Source Constraints-Driven Partitioner

TritonPart (or K-SpecPart) is an open-source constraints-driven partitioner.  It can be used to partition a hypergraph or a gate-level netlist.

## Highlights
- Start of the art multiple-constraints driven partitioning “multi-tool”
- Optimizes cost function based on user requirement
- Permissive open-source license
- Solves multi-way partitioning with following features:
  - Multidimensional real-value weights on vertices and hyperedges
  - Multilevel coarsening and refinement framework
  - Fixed vertices constraint
  - Timing-driven partitioning framework 
  - Group constraint: Groups of vertices need to be in same block
  - Embedding-aware partitioning
  
## Dependency
We use Google OR-Tools as our ILP solver.  Please install Google OR-Tools following the [instructions](https://developers.google.com/optimization/install).

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Partition Netlist

```tcl
triton_part_hypergraph
    -hypergraph_file hypergraph_file  
    -num_parts num_parts  
    -balance_constraint balance_constraint 
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
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-num_parts` | number of partitions (default 2) |
| `-balance_constraint` | allowed imbalance between blocks (default 1.0) |
| `-seed` | random seed (default 0) |
| `-vertex_dimension` | number of vertices in the hypergraph (default 1) |
| `-hyperedge_dimension` | number of hyperedges in hypergraph (default 1) |
| `-placement_dimension` | number of dimensions for canvas, if placement information is provided (default 0) |
| `-hypergraph_file` | path to hypergraph file |
| `-fixed_file` | path to fixed vertices constraint file |
| `-community_file` | path to `community` attributes file to guide the partitioning process |
| `-group_file` | path to `stay together` attributes file |
| `-placement_file` | placement information file, each line corresponds to a group fixed vertices, community and placement attributes following the [hMETIS](https://course.ece.cmu.edu/~ee760/760docs/hMetisManual.pdf) format. |
| `-e_wt_factors` | hyperedge weight factor |
| `-v_wt_factors` | vertex weight factors |
| `-placement_wt_factors` | placement weight factors |
| `-thr_coarsen_hyperedge_size_skip` | threshold for ignoring large hyperedge (default 200) |
| `-thr_coarsen_vertices` | number of vertices of coarsest hypergrpah (default 10) |
| `-thr_coarsen_hyperedges` | number of vertices of coarsest hypergraph (default 50) |
| `-coarsening_ratio` | coarsening ratio of two adjacent hypergraphs (default 1.6) |
| `-max_coarsen_iters` | number of iterations (default 30) |
| `-adj_diff_ratio` | minimum difference of two adjacent hypergraphs (default 0.0001) |
| `-min_num_vertices_each_part` | minimum number of vertices in each partition (default 4) |
| `-num_initial_solutions` | number of initial solutions (default 50) |
| `-num_best_initial_solutions` | number of top initial solutions to filter out (default 10) |
| `-refiner_iters` | refinement iterations (default 10) |
| `-max_moves` | the allowed moves for each pass of Fiduccia-Mattheyes (FM) algorithm or greedy refinement (default 60) |
| `-early_stop_ratio` | describes ratio $e$ where if the $n_{moved vertices} > n_{vertices} * e$, exit current FM pass. the intution behind this being most of the gains are achieved by the first few FM moves. (default 0.5) |
| `-total_corking_passes` | maximum level of traversing the buckets to solve the "corking effect" (default 25) |
| `-v_cycle_flag` | disables v-cycle is used to refine partitions (default true) |
| `-max_num_vcycle` | maximum number of vcycles(default 1) |
| `-num_coarsen_solutions` | number of coarsening solutions with different randoms seed (default 3) |
| `-num_vertices_threshold_ilp` | describes threshold $t$, number of vertices used for integer linear programming (ILP) partitioning. if $n_{vertices} > t$, do not use ILP-based partitioning.(default 50) |
| `-global_net_threshold` | if the net is larger than this, it will be ignored by TritonPart (default 1000) |

### Evaluate Hypergraph Partition

```tcl
evaluate_hypergraph_solution
  -num_parts num_parts
  -balance_constraint balance_constraint
  -hypergraph_file hypergraph_file
  -solution_file solution_file
  [-vertex_dimension vertex_dimension]
  [-hyperedge_dimension hyperedge_dimension]
  [-fixed_file fixed_file]
  [-group_file group_file]
  [-e_wt_factors e_wt_factors]
  [-v_wt_factors v_wt_factors] 
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-num_parts` | number of partitions (default 2) |
| `-balance_constraint` | allowed imbalance between blocks (default 1.0) |
| `-vertex_dimension` | number of vertices in the hypergraph (default 1) |
| `-hyperedge_dimension` | number of hyperedges in hypergraph (default 1) |
| `-hypergraph_file` | path to hypergraph file |
| `-solution_file` | path to solution file|
| `-fixed_file` | path to fixed vertices constraint file |
| `-group_file` | path to `stay together` attributes file |
| `-e_wt_factors` | hyperedge weight factor |
| `-v_wt_factors` | vertex weight factor |


### Partition Netlist 

```tcl
triton_part_design
    [-num_parts num_parts]
    [-balance_constraint balance_constraint]
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
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-num_parts` | number of partitions (default 2) |
| `-balance_constraint` | allowed imbalance between blocks (default 1.0) |
| `-seed` | random seed (default 1) |
| `-timing_aware_flag` | enable timing-driven mode (default true) |
| `-top_n` | (default 1000) |
| `-placement_flag` | (default false) |
| `-fence_flag ` | (default false) |
| `-fence_lx ` | (default 0.0) |
| `-fence_ly ` | (default 0.0) |
| `-fence_ux ` | (default 0.0) |
| `-fence_uy ` | (default 0.0) | 
| `-fixed_file` | path to fixed vertices constraint file |
| `-community_file` | path to `community` attributes file to guide the partitioning process |
| `-group_file` | path to `stay together` attributes file |
| `-solution_file` | path to solution file |
| `-net_timing_factor` |  (default 1.0)|
| `-path_timing_factor` |  (default 1.0)|
| `-path_snaking_factor` |  (default 1.0)|
| `-timing_exp_factor` |  (default 1.0)|
| `-extra_delay` |  (default 1e-9)|
| `-guardband_flag` |  (default false)|
| `-e_wt_factors` | hyperedge weight factor |
| `-v_wt_factors` | vertex weight factor |
| `-placement_wt_factors` | placement weight factor |
| `-thr_coarsen_hyperedge_size_skip` | threshold for ignoring large hyperedge (default 1000) |
| `-thr_coarsen_vertices` | number of vertices of coarsest hypergrpah (default 10) |
| `-thr_coarsen_hyperedges` | number of vertices of coarsest hypergraph (default 50) |
| `-coarsening_ratio` | coarsening ratio of two adjacent hypergraphs (default 1.5) |
| `-max_coarsen_iters` | number of iterations (default 30) |
| `-adj_diff_ratio` | minimum difference of two adjacent hypergraphs (default 0.0001) |
| `-min_num_vertices_each_part` | minimum number of vertices in each partition (default 4) |
| `-num_initial_solutions` | number of initial solutions (default 100) |
| `-num_best_initial_solutions` | number of top initial solution to filter out (default 10) |
| `-refiner_iters` | refinment iterations (default 10) |
| `-max_moves` | the allowed moves for each pass of Fiduccia-Mattheyes (FM) algorithm or greedy refinement (default 100) |
| `-early_stop_ratio` | describes ratio $e$ where if the $n_{moved vertices} > n_{vertices} * e$, exit current FM pass. the intution behind this being most of the gains are achieved by the first few FM moves. (default 0.5) |
| `-total_corking_passes` | maximum level of traversing the buckets to solve the "corking effect" (default 25) |
| `-v_cycle_flag` | disables v-cycle is used to refine partitions (default true) |
| `-max_num_vcycle` | maximum number of vcycles (default 1) |
| `-num_coarsen_solutions` | number of coarsening solutions with different randoms seed (default 4) |
| `-num_vertices_threshold_ilp` | describes threshold $t$, number of vertices used for integer linear programming (ILP) partitioning. if $n_{vertices} > t$, do not use ILP-based partitioning. (default 50) |
| `-global_net_threshold` | if the net is larger than this, it will be ignored by TritonPart (default 1000) |


### Evaluation Netlist Partition

```tcl
evaluate_part_design_solution
    [-num_parts num_parts]
    [-balance_constraint balance_constraint]
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
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-num_parts` |  |
| `-balance_constraint` |  |
| `-timing_aware_flag` |  |
| `-top_n` |  |
| `-fence_flag ` |  |
| `-fence_lx ` |  |
| `-fence_ly ` |  |
| `-fence_ux ` |  |
| `-fence_uy ` |  |
| `-fixed_file` |  |
| `-community_file` |  |
| `-group_file` |  |
| `-hypergraph_file` |  |
| `-hypergraph_int_weight_file` |  |
| `-solution_file` |  |
| `-net_timing_factor` |  |
| `-path_timing_factor` |  |
| `-path_snaking_factor` |  |
| `-timing_exp_factor` |  |
| `-extra_delay` |  |
| `-guardband_flag` |  |
| `-e_wt_factors` |  |
| `-v_wt_factors` |  |

### Write Partition to Verilog

```tcl
write_partition_verilog
    [-port_prefix prefix]
    [-module_suffix suffix]
    [file]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `port_prefix` |  |
| `module_suffix` |  |

### Read Partition file

```tcl
read_partitioning
    -read_file name
    [-instance_map_file file_path]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-read_file` |  |
| `-instance_map_file` |  |


## Example Scripts

### How to partition a hypergraph in the way you would using hMETIS (min-cut partitioning)

```tcl
triton_part_hypergraph -hypergraph_file des90.hgr -num_parts 5 -balance_constraint 2 -seed 2
```
You can also check the provided example [here](./examples/min-cut-partitioning).

### How to perform the embedding-aware partitioning

```tcl
set num_parts 2
set balance_constraint 2
set seed 0
set design sparcT1_chip2
set hypergraph_file "${design}.hgr"
set placement_file "${design}.hgr.ubfactor.2.numparts.2.embedding.dat"
set solution_file "${design}.hgr.part.${num_parts}"

triton_part_hypergraph  -hypergraph_file $hypergraph_file -num_parts $num_parts \
                        -balance_constraint $balance_constraint \
                        -seed $seed  \
                        -placement_file ${placement_file} -placement_wt_factors { 0.00005 0.00005 } \
                        -placement_dimension 2

```

You can find the provided example [here](./examples/embedding-aware-partitioning).


### How to partition a netlist

```tcl
# set technology information
set ALL_LEFS “list_of_lefs”
set ALL_LIBS “list_of_libs”
# set design information
set design “design_name”
set top_design “top_design”
set netlist “netlist.v”
set sdc “timing.sdc”
foreach lef_file ${ALL_LEFS} {
  read_lef $lef_file
}
foreach lib_file ${ALL_LIBS} {
  read_lib $lib_file
}
read_verilog $netlist
link_design $top_design
read_sdc $sdc

set num_parts 5
set balance_constraint 2
set seed 0
set top_n 100000
# set the extra_delay_cut to 20% of the clock period
# the extra_delay_cut is introduced for each cut hyperedge
set extra_delay_cut 9.2e-10  
set timing_aware_flag true
set timing_guardband true
set part_design_solution_file "${design}_part_design.hgr.part.${num_parts}"

##############################################################################################
### TritonPart with slack progagation
##############################################################################################
puts "Start TritonPart with slack propagation"
# call triton_part to partition the netlist
triton_part_design -num_parts $num_parts -balance_constraint $balance_constraint \
                   -seed $seed -top_n $top_n \
                   -timing_aware_flag $timing_aware_flag -extra_delay $extra_delay_cut \
                   -guardband_flag $timing_guardband \
                   -solution_file $part_design_solution_file 
```

You can find the provided example [here](./examples/timing-aware-partitioning).

## References
1. Bustany, I., Kahng, A. B., Koutis, I., Pramanik, B., & Wang, Z. (2023). K-SpecPart: A Supervised Spectral Framework for Multi-Way Hypergraph Partitioning Solution Improvement. arXiv preprint arXiv:2305.06167. [(.pdf)](https://arxiv.org/pdf/2305.06167)
1. Bustany, I., Kahng, A. B., Koutis, I., Pramanik, B., & Wang, Z. (2022, October). SpecPart: A supervised spectral framework for hypergraph partitioning solution improvement. In Proceedings of the 41st IEEE/ACM International Conference on Computer-Aided Design (pp. 1-9). [(.pdf)](https://dl.acm.org/doi/pdf/10.1145/3508352.3549390)


## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.

