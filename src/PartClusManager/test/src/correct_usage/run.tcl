read_lef "merged.lef"
read_def "aes.def"

# Invalid command usages
partition_netlist 
partition_netlist -tool cacho
partition_netlist -tool chaco -num_starts 10
partition_netlist -tool chaco -num_starts 10 -num_partitions 1
partition_netlist -tool chaco -num_starts 10 -num_partitions 100000
exit
