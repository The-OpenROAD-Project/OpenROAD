source "helpers.tcl"
source flow_helpers.tcl

read_liberty "Nangate45/Nangate45_typ.lib"
read_lef Nangate45/Nangate45.lef
read_verilog gcd.v
link_design gcd

read_sdc gcd_nangate45.sdc

set part_file [make_result_file partition_gcd.part]
set graph_file [make_result_file partition_gcd.graph]
set paths_file [make_result_file partition_gcd.paths]
set part_v [make_result_file partition_gcd.v]

triton_part_design -num_parts 5 -seed 0 -timing_aware_flag true

