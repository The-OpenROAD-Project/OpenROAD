# left/right rows too far away from instance -> fails
source helpers.tcl
read_lef Nangate45.lef
read_def fragmented_row03.def
detailed_placement
check_placement -verbose

set def_file [make_result_file fragmented_row03.def]
write_def $def_file
diff_file $def_file fragmented_row03.defok
