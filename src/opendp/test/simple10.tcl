# 1 inst upper right of core
source helpers.tcl
read_lef Nangate45.lef
read_def simple10.def
detailed_placement
check_placement

set def_file [make_result_file simple10.def]
write_def $def_file
diff_file simple10.defok $def_file
