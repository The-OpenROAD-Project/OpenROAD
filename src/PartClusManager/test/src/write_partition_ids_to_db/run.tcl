read_lef "merged.lef"
read_def "aes.def"

partition_netlist -tool chaco -num_partitions 2 -num_starts 1
evaluate_partitioning -partition_ids 0 -evaluation_function hyperedges
write_partitioning_to_db -partitioning 0 -dump_to_file "part_ids.txt"
exit
