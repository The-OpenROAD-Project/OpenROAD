# set_placement_padding 2
source "helpers.tcl"
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def pad09.def
# check -masters arg parsing
set_placement_padding -global -left 2 -right 2
check_placement

set def_file [make_result_file pad09.def]
write_def $def_file
diff_file pad09.defok $def_file
