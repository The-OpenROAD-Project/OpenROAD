# set_placement_padding -masters
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def simple01.def
# check -masters arg parsing
set_placement_padding -masters BUF_X4 -left 5
set_placement_padding -masters {BUF_X1 BUF_X4} -left 5
set_placement_padding -masters [get_lib_cells BUF_X*] -left 5
detailed_placement
check_placement

set def_file [make_result_file pad08.def]
write_def $def_file
diff_file pad08.defok $def_file
