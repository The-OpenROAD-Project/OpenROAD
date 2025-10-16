# std cell abutting block on the right with left padding
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef extra.lef
read_def check6.def
set_placement_padding -global -right 1
detailed_placement
check_placement -verbose

set def_file [make_result_file pad06.def]
write_def $def_file
diff_file pad06.defok $def_file
