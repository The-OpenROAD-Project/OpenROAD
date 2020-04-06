# PartClusManager

PartClusManager is a tool that runs partitioning/clustering on a specific netlist. It provides a wrapper of three well-known open-source tools: Chaco, GPMetis and MLPart.

### Usage

PartClusManager offers three commands: partition_netlist, evaluate_partitioning and write_partitioning_to_db.

**partition_netlist**: Divides the netlist into N partitions and returns the id (partition_id) of the partitioning solution. The command may be called many times with different parameters. Each time, the command will generate a new solution.

The following tcl snippet shows how to call partition_netlist.

```
read_lef "mylef.lef"
read_def "mydef.def"

partition_netlist   -tool name \
                    -target_partitions value \
                    [-graph_model name] \
                    [-clique_threshold value] \
                    [-weight_model name] \
                    [-max_edge_weight value] \
                    [-max_vertex_weight range] \
                    [-num_starts value] \
                    [-seeds value] \
                    [-balance_constraint value] \
                    [-coarsening_ratio value] \
                    [-coarsening_vertices value] \
                    [-enable_term_prop value] \
                    [-cut_hop_ratio value] \
                    [-architecture value] \

```

Argument description:
- ``tool`` (Mandatory) defines the partitioning tool. Can be "chaco", "gpmetis" or "mlpart".
- ``target_partitions`` (Mandatory) defines the number of target partitions. Can be any interger higher than 1.
- ``graph_model`` is the hypergraph to graph decomposition approach. Can be "clique", "star" or "hybrid".
- ``clique_threshold`` is the max degree of a net decomposed with the clique net model. If using the clique net model, nets with a degree higher than threshold are ignored. In the hybrid net model, nets with a degree higher than threshold are decomposed using the star model.
- ``weight_model`` is the edge weight scheme for the graph model of the netlist. Can be any interger from 1 to 7.
- ``max_edge_weight`` defines the max weight of an edge.
- ``max_vertex_weight`` defines the max weight of a vertex.
- ``num_starts`` is the number of solutions generated with different random seeds.
- ``seeds`` is the number of solutions generated with set seeds.
- ``balance_constraint`` is the max vertex area percentage difference among partitions. E.g., a 50% difference means one partition can hold up to 25% larger area during a 2-way partition.
- ``coarsening_ratio`` defines the minimal acceptable reduction in the number of vertices in the coarsening step.
- ``coarsening_vertices`` defines the maximum number of vertices that the algorithm aims to coarsen a graph to. 
- ``enable_term_prop`` enables Terminal Propagation, which aims to improve data locality. This adds constraints to the KL algorithm, as seen in the Dunlop and Kernighan Algorithm. Improves the number of edge cuts and terminals with a minimal hit on run-time.
- ``cut_hop_ratio`` controls the relative importance of generating a new cut edge versus increasing the interprocessor distance associated with an existing cut edge (data locality x cut edges tradeoff).
- ``architecture`` defines the topology (for parallel processors) to be used in the partitioning. These can be 2D or 3D topologies, and they define the total number of target partitions.

**evaluate_partitioning**: Evaluates the partitioning solution(s) based on a specific objective function. This function is run for each partitioning solution that is supplied in the  partition_ids parameter (return value from partition_netlist) and returns the best one depending on the specified objective (i.e., metric).

The following tcl snippet shows how to call evaluate_partitioning.

```
read_lef "mylef.lef"
read_def "mydef.def"

set id [partition_netlist   -tool chaco \
                            -target_partitions 4 \
                            -num_starts 5           ]

evaluate_partitioning   -partition_ids $id \
                        -evaluation_function function \
```

Argument description:
- ``partition_ids`` (Mandatory) are the partitioning solution id. These are the return values from the partition_netlist command. They can be a list of values or only one ID.
- ``evaluation_function`` (Mandatory) is the objective function that is evaluated for each partitioning solution. Can be "terminals", "hyperedges", "size", "area", "runtime", or "hops".

**write_partitioning_to_db**: Writes the partition id of each instance (i.e. the cluster that contains the instance) to the DB as a property called “partitioning_id.”

The following tcl snippet shows how to call write_partitioning_to_db.

```
read_lef "mylef.lef"
read_def "mydef.def"

set id [partition_netlist   -tool chaco \
                            -target_partitions 4 \
                            -num_starts 5           ]

evaluate_partitioning   -partition_ids $id \
                        -evaluation_function "hyperedges" \

write_partitioning_to_db    -partitioning_id $id \
                            [-dump_to_file name] \
```

Argument description:
- ``partitioning_id`` (Mandatory) is the partitioning solution id. These are the return values from the partition_netlist command.
- ``dump_to_file`` is the file where the vertex assignment results will be saved. These consist of lines with a vertex name (e.g. an instance) and the partition it is part of.

### Third party packages

[Chaco](https://cfwebprod.sandia.gov/cfdocs/CompResearch/templates/insert/softwre.cfm?sw=36).

[GPMetis](http://glaros.dtc.umn.edu/gkhome/metis/metis/overview).

[MLPart](https://vlsicad.ucsd.edu/GSRC/bookshelf/Slots/Partitioning/MLPart/).

### Authors

PartClusManager is written by Mateus Fogaça and Isadora Oliveira from the Federal University of Rio Grande do Sul (UFRGS), Brazil, and Marcelo Danigno from the Federal University of Rio Grande (FURG), Brazil.

Mateus's and Isadora's advisor is Prof. Ricardo Reis; Marcelo's advisor is Prof. Paulo Butzen.

Many guidance provided by:
*  Andrew B. Kahng
*  Tom Spyrou
