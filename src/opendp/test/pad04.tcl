# 10 inst placement legal (should not move anything)
source helpers.tcl
read_lef Nangate45.lef
read_def pad04.def
set_placement_padding -global -left 2 -right 2
detailed_placement
check_placement

set def_file [make_result_file pad04.def]
write_def $def_file
diff_file pad04.defok $def_file
