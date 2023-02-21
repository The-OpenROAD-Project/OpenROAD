# TritonPart : An Open-Source Constraints-Driven Partitioner

TritonPart is an open-source constraints-driven partitioner.  It can be used to partition a hypergraph or a gate-level netlist.

## Features
- Start of the art multiple-constraints driven partitioning “multi-tool”
- Optimizes cost function based on user requirement
- Permissive open-source license
- Solves multi-way partitioning with following features:
  - Multidimensional real-value weights on vertices and hyperedges
  - Multilevel coarsening and refinement framework
  - Fixed vertices
  - Timing-driven partitioning framework 
  - Community constraints: Groups of vertices need to be in same partition
  
 
## Dependency
We use Google OR-Tools as our ILP solver.  Please install Google OR-Tools following the [instructions](https://developers.google.com/optimization/install).


## How to partition a hypergraph in the way you would using hMETIS
``` shell
triton_part_hypergraph -hypergraph_file des90.hgr -num_parts 5 -balance_constraint 2 -seed 2
```

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
triton_part_design –num_parts 2 –balance_constraint 2 –seed 0
```

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
