source "helpers.tcl"
source flow_helpers.tcl

read_liberty "Nangate45/Nangate45_typ.lib"
read_lef Nangate45/Nangate45.lef
read_verilog gcd.v
link_design gcd

read_sdc gcd_nangate45.sdc

set part_file [make_result_file partition_gcd.part]

triton_part_design -solution_file $part_file

diff_files partition_gcd.partok $part_file
