source "helpers.tcl"
source flow_helpers.tcl

read_liberty "Nangate45/Nangate45_typ.lib"
read_lef Nangate45/Nangate45.lef
read_verilog gcd.v
link_design gcd

set part_v [make_result_file read_part.v]

read_partitioning -read_file read_part.part
write_partition_verilog $part_v

diff_files read_part.vok $part_v
