# set_padding -global -right
source helpers.tcl
read_lef Nangate45.lef
read_def simple03.def
set_placement_padding -global -right 5

set def_file [make_result_file pad03.def]
write_def $def_file
diff_file pad03.defok $def_file
