# PAR Debug Messages

PAR debug messages are divided in the following single-level groups:
- 8 groups according general PAR flow;
- 2 groups for the top level TritonPart methods.

## General PAR Flow Groups

### Multilevel Partitioning
- Group Name: `multilevel_partitioning`
- Description: Messages related to the multilevel partitioning methodology function used in TritonPart.

### Coarsening
- Group Name: `coarsening`
- Description: Messages related to the coarsening step of the TritonPart framework flow.

### Initial Partitioning
- Group Name: `initial_partitioning`
- Description: Messages related to the initial partitioning step of the TritonPart framework flow.

### Refinement
- Group Name: `refinement`
- Description: Messages related to the refinement step of the TritonPart framework flow.

### Cut-Overlay Clustering
- Group Name: `cut_overlay_clustering`
- Description: Although the actual step in TritonPart framework is Cut-Overlay Clustering and Partitioning, these messages are related to the Cut-Overlay Clustering, because general partitioning is done by a class used by other mechanisms ("partitioning" group).

### V-Cycle Refinement
- Group Name: `v_cycle_refinement`
- Description: Messages related to the V-Cycle Refinement step of the TritonPart framework flow.

### Partitioning
- Group Name: `partitioning`
- Description: Messages related to the partitioning mechanism used by different functions.

### Evaluation
- Group Name: `evaluation`
- Description: Messages related to the evaluation mechanism used by different functions.

## Top Level TritonPart Methods

### HyperGraph Partitioning and Evaluation
- Group Name: `hypergraph`
- Description: Messages related to the main functions triggered by .tcl commands for partitioning and evaluation of a hypergraph.

### Netlist Partitioning and Evaluation
- Group Name: `netlist`
- Description: Messages related to the main functions triggered by .tcl commands for partitioning and evaluation of a netlist.
