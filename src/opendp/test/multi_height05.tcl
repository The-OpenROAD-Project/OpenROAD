# an example from sky130 with power rails having multiple ports (on li1 and met1)
source "helpers.tcl"
read_lef sky130hs/sky130hs.tlef
read_lef multi_height05.lef
read_def multi_height05.def
detailed_placement
check_placement

set def_file [make_result_file multi_height05.def]
write_def $def_file
diff_file $def_file multi_height05.defok
