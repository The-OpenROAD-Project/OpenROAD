# 1 inst lower left of core
source helpers.tcl
read_lef Nangate45.lef
read_def simple01.def
detailed_placement
check_placement

set def_file [make_result_file simple01.def]
write_def $def_file
diff_file simple01.defok $def_file
