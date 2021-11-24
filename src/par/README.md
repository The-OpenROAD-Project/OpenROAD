# PartitionMgr

PartitionMgr is a tool that performs partitioning/clustering on a specific
netlist. It provides a wrapper of three well-known permissively open-sourced tools:
Chaco, GPMetis and MLPart.

## Commands

PartitionMgr offers six commands: partition_netlist, evaluate_partitioning,
write_partitioning_to_db, cluster_netlist, write_clustering_to_db and
report_netlist_partitions.

### `partition_netlist`

Divides the netlist into N partitions and returns
the id (partition_id) of the partitioning solution. The command may be
called many times with different parameters. Each time, the command will
generate a new solution.

The following Tcl snippet shows how to call partition_netlist:

```
partition_netlist -tool <name>
                  -num_partitions <value>
                  [-graph_model <name>]
                  [-clique_threshold <value>]
                  [-weight_model <name>]
                  [-max_edge_weight <value>]
                  [-max_vertex_weight range]
                  [-num_starts <value>]
                  [-random_seed <value>]
                  [-seeds <value>]
                  [-balance_constraint <value>]
                  [-coarsening_ratio <value>]
                  [-coarsening_vertices <value>]
                  [-enable_term_prop <value>]
                  [-cut_hop_ratio <value>]
                  [-architecture <value>]
                  [-refinement <value>]
                  [-partition_id <value>]
```

Argument description:

-   `tool` (Mandatory) defines the partitioning tool. Can be "chaco",
    "gpmetis" or "mlpart".
-   `num_partitions` (Mandatory) defines the final number of partitions. Can
    be any integer greater than 1.
-   `graph_model` is the hypergraph to graph decomposition approach. Can be
    "clique", "star" or "hybrid".
-   `clique_threshold` is the maximum degree of a net decomposed with the clique
    net model. If using the clique net model, nets with degree higher than
    the threshold are ignored. In the hybrid net model, nets with degree higher
    than the threshold are decomposed using the star model.
-   `weight_model` is the edge weight scheme for the graph model of the
    netlist. Can be any integer from 1 to 7.
-   `max_edge_weight` defines the maximum weight of an edge.
-   `max_vertex_weight` defines the maximum weight of a vertex.
-   `num_starts` is the number of solutions generated with different
    random seeds.
-   `random_seed` is the seed used when generating new random set seeds.
-   `seeds` is the number of solutions generated with set seeds.
-   `balance_constraint` is the maximum vertex area percentage difference between two
    partitions. E.g., a 50% difference means that neither partition in a 2-way partitioning can contain less than 25% or more than 75% of the total vertex area in the netlist.
-   `coarsening_ratio` defines the minimal acceptable reduction in the
    number of vertices in the coarsening step.
-   `coarsening_vertices` defines the maximum number of vertices that the
    algorithm aims to have in its most-coarsened graph or hypergraph.
-   `enable_term_prop` enables terminal propagation (Dunlop and Kernighan, 1985), which aims to improve
    data locality. This adds constraints to the Kernighan-Lin (KL) algorithm. Improves the number of edge cuts and
    terminals, at the cost of additional runtime.
-   `cut_hop_ratio` controls the relative importance of generating a new
    cut edge versus increasing the interprocessor distance associated with an
    existing cut edge (tradeoff of data locality versus cut edges). This is largely specific to Chaco.
-   `architecture` defines the topology (for parallel processors) to be used
    in the partitioning. These can be 2D or 3D topologies, and they define the
    total number of partitions. This is largely specific to Chaco.
-   `refinement` specifies how many times a KL refinement is run. Incurs a modest
    runtime hit, but can generate better partitioning results.
-   `partition_id` is the partition_id (output from partition_netlist)
    from a previous computation. This is used to generate better results based
    on past results or to run further partitioning.

### `evaluate_partitioning`

Evaluates the partitioning solution(s) based on
a specific objective function. This function is run for each partitioning
solution that is supplied in the partition_ids parameter (return value
from partition_netlist) and returns the best partitioning solution according to the specified
objective (i.e., metric).

```
evaluate_partitioning -partition_ids <id> -evaluation_function <function>
```

Argument description:

-   `-partition_ids` (mandatory) are the partitioning solution id's. These
    are the return values from the partition_netlist command. They can be a
    list of values or only one id.
-   `-evaluation_function` (mandatory) is the objective function that is
    evaluated for each partitioning solution. It can be `terminals`, `hyperedges`,
    `size`, `area`, `runtime`, or `hops`.

The following Tcl snippet shows how to call evaluate_partitioning:

``` tcl
set id [ partition_netlist -tool chaco -num_partitions 4 -num_starts 5 ]

evaluate_partitioning -partition_ids $id -evaluation_function function
```

### `write_partitioning_to_db`

Writes the partition id of each instance (i.e., the cluster that contains
the instance) to the DB as a property called `partitioning_id`.

The following Tcl snippet shows how to call `write_partitioning_to_db`:

```
write_partitioning_to_db -partitioning_id <id> [-dump_to_file <file>]
```

Argument description:

-   `-partitioning_id` (Mandatory) is the partitioning solution id. These
    are the return values from the partition_netlist command.
-   `-dump_to_file` is the file where the vertex assignment results will
    be saved. The assignment results consist of lines that each contain a vertex name (e.g. an instance)
    and the partition it is part of.

Another Tcl snippet showing the use of `write_partitioning_to_db` is:

``` tcl
set id [partition_netlist -tool chaco -num_partitions 4 -num_starts 5]
evaluate_partitioning -partition_ids $id -evaluation_function "hyperedges"
write_partitioning_to_db -partitioning_id $id -dump_to_file "file.txt"
```

### `write_partition_verilog`

Writes the partitioned network to a Verilog file containing modules for
each partition.

```
write_partition_verilog -partitioning_id <id>
                        [-port_prefix <prefix>]
                        [-module_suffix <suffix>]
                        <file.v>
```

Argument description:

-   `partitioning_id` (Mandatory) is the partitioning solution id. These
    are the return values from the partition_netlist command.
-   `filename` (Mandatory) is the path to the Verilog file.
-   `port_prefix` is the prefix to add to the internal ports created during
    partitioning; default is `partition_`.
-   `module_suffix` is the suffix to add to the submodules; default is `_partition`.

The following Tcl snippet shows how to call write_partition_verilog:

``` tcl
set id [partition_netlist -tool chaco -num_partitions 4 -num_starts 5 ]
evaluate_partitioning -partition_ids $id -evaluation_function "hyperedges"
write_partition_verilog -partitioning_id $id -port_prefix prefix -module_suffix suffix filename.v
```

### `cluster_netlist`

Divides the netlist into N clusters and returns the id (cluster_id) of the
clustering solution. The command may be called many times with different
parameters. Each time, the command will generate a new solution.  (Note that when we partition
a netlist, we typically seek N = O(1) partitions. On the other hand, when we cluster a netlist, we typically
seek N = Theta(|V|) clusters.)

```
cluster_netlist -tool name
                [-coarsening_ratio value]
                [-coarsening_vertices value]
                [-level value]
```

Argument description:

-   `tool` (Mandatory) defines the multilevel partitioning tool whose recursive coarsening is used to induce a clustering. Can be "chaco",
    "gpmetis", or "mlpart".
-   `coarsening_ratio` defines the minimal acceptable reduction in the
    number of vertices in the coarsening step.
-   `coarsening_vertices` defines the maximum number of vertices that the
    algorithm aims to coarsen a graph to.
-   `level` defines which is the level of clustering to return.

### `write_clustering_to_db`

Writes the cluster id of each instance (i.e., the cluster that contains the
instance) to the DB as a property called `cluster_id`.

```
write_clustering_to_db -clustering_id <id> [-dump_to_file <name>]
```

Argument description:

-   `clustering_id` (Mandatory) is the clustering solution id. These are
    the return values from the cluster_netlist command.
-   `dump_to_file` is the file where the vertex assignment results will
    be saved. The assignment results consist of lines that each contain a vertex name (e.g. an instance)
    and the cluster it is part of.

The following Tcl snippet shows how to call write_clustering_to_db:

```
set id [cluster_netlist -tool chaco -level 2 ]
write_clustering_to_db -clustering_id $id -dump_to_file name
```

### `report_netlist_partitions`

Reports the number of partitions for a specific partition_id and the number
of vertices present in each one.

```
report_netlist_partitions -partitioning_id <id>
```

Argument description:

-   `partitioning_id` (Mandatory) is the partitioning solution id. These
    are the return values from the partition_netlist command.

The following Tcl snippet shows how to call report_netlist_partitions:

```
set id [partition_netlist -tool chaco -num_partitions 4 -num_starts 5 ]
evaluate_partitioning -partition_ids $id -evaluation_function "hyperedges"
report_netlist_partitions -partitioning_id $id
```

## Regression tests

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+PartitionMgr+in%3Atitle)
about this tool.


## External references

-   [Chaco](https://cfwebprod.sandia.gov/cfdocs/CompResearch/templates/insert/softwre.cfm?sw=36).
-   [GPMetis](http://glaros.dtc.umn.edu/gkhome/metis/metis/overview).
-   [MLPart](https://vlsicad.ucsd.edu/GSRC/bookshelf/Slots/Partitioning/MLPart/).


## Authors

PartitionMgr is written by Mateus Fogaça and Isadora Oliveira from the
Federal University of Rio Grande do Sul (UFRGS), Brazil, and Marcelo Danigno
from the Federal University of Rio Grande (FURG), Brazil.

Mateus's and Isadora's advisor is Prof. Ricardo Reis; Marcelo's advisor is
Prof. Paulo Butzen.

Many guidance provided by:

-    Andrew B. Kahng
-    Tom Spyrou

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
