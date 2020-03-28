read_lef "merged.lef"
read_def "aes.def"

# Invalid command usages
partition_netlist 
partition_netlist -tool cacho
partition_netlist -tool chaco
partition_netlist -tool chaco -target_partitions 1
partition_netlist -tool chaco -target_partitions 100000
exit
