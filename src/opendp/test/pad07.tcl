# set_placement_padding inst -left
source helpers.tcl
read_lef Nangate45.lef
read_def simple01.def
set_placement_padding -left 5 _277_
detailed_placement
check_placement

set def_file [make_result_file pad0.7def]
write_def $def_file
diff_file pad01.defok $def_file
