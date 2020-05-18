read_lef "merged.lef"
read_def "aes.def"
set id [partition_netlist -tool chaco -weight_model 1 -coarsening_ratio 0.6 -enable_term_prop 0 -balance_constraint 10 -num_partitions 2 -graph_model hybrid -num_starts 1]
set id2 [partition_netlist -tool chaco -weight_model 4 -coarsening_ratio 0.8 -enable_term_prop 1 -balance_constraint 20 -num_partitions 2 -graph_model clique -num_starts 1]
set id3 [partition_netlist -tool chaco -weight_model 7 -coarsening_ratio 0.75 -enable_term_prop 0 -balance_constraint 5 -num_partitions 2 -graph_model star -num_starts 1]
evaluate_partitioning -partition_ids "${id} ${id2} ${id3}" -evaluation_function terminals
