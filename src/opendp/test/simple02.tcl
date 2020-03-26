# 1 inst in core off grid
source helpers.tcl
read_lef Nangate45.lef
read_def simple02.def
detailed_placement
check_placement

set def_file [make_result_file simple02.def]
write_def $def_file
diff_file simple02.defok $def_file
