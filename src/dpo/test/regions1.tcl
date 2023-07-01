# regions1
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def regions1.def
improve_placement
check_placement

set def_file [make_result_file regions1.def]
write_def $def_file
diff_file $def_file regions1.defok
