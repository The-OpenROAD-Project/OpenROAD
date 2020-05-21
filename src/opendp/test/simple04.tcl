# 10 inst placement legal (should not move anything)
source helpers.tcl
read_lef Nangate45.lef
read_def simple04.def
detailed_placement
check_placement

set def_file [make_result_file simple04.def]
write_def $def_file
diff_file simple04.defok $def_file
