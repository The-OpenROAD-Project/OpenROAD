# A hard blockage (to avoid) and a soft blockage (to ignore)
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def blockage01.def
detailed_placement
check_placement

set def_file [make_result_file blockage01.def]
write_def $def_file
diff_file blockage01.defok $def_file
