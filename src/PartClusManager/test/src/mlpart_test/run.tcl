read_lef "merged.lef"
read_def "aes.def"
set id [partition_netlist -tool mlpart -num_partitions 4 -num_starts 2]
evaluate_partitioning -partition_ids "${id}" -evaluation_function hyperedges
