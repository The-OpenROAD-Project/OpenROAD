# std cell abutting on the right set_placement_padding -right
source helpers.tcl
read_lef Nangate45.lef
read_lef extra.lef
read_def check6.def
set_placement_padding -global -right 1
detailed_placement
check_placement -verbose

set def_file [make_result_file pad05.def]
write_def $def_file
diff_file pad05.defok $def_file
