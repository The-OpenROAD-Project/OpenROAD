# set_placement_padding with abutting blocks
source helpers.tcl
read_lef Nangate45.lef
read_lef block1.lef
read_def pad05.def
set_placement_padding -global -left 5 -right 5
detailed_placement
check_placement

set def_file [make_result_file pad01.def]
write_def $def_file
diff_file pad01.defok $def_file
