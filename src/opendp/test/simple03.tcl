# 1 inst upper right of core
source helpers.tcl
read_lef Nangate45.lef
read_def simple03.def
detailed_placement
check_placement

set def_file [make_result_file simple03.def]
write_def $def_file
diff_file simple03.defok $def_file
