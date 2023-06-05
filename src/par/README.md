# TritonPart : An Open-Source Constraints-Driven Partitioner

TritonPart is an open-source constraints-driven partitioner.  It can be used to partition a hypergraph or a gate-level netlist.

## Features
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

## How to partition a hypergraph in the way you would using hMETIS (min-cut partitioning)
``` shell
triton_part_hypergraph -hypergraph_file des90.hgr -num_parts 5 -balance_constraint 2 -seed 2
```
You can also check the provided example [here](./examples/min-cut-partitioning).

## How to perform the embedding-aware partitioning
``` shell
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


## How to partition a netlist
``` shell
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


## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.

