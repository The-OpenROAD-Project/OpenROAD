# set_placement_padding -instances
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def simple01.def
set_placement_padding -instances _277_ -left 5
set_placement_padding -instances [get_cells _277_] -left 5
detailed_placement
check_placement

set def_file [make_result_file pad07.def]
write_def $def_file
diff_file pad07.defok $def_file
