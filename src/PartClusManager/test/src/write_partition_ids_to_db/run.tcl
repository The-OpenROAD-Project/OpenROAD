read_lef "merged.lef"
read_def "aes.def"

# Invalid command usages
partition_netlist -tool chaco -target_partitions 2 -num_starts 1
write_partitioning_to_db -partitioning 0 -dump_to_file "part_ids.txt"
exit
