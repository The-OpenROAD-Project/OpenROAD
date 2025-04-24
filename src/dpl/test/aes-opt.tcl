# aes-opt (low utilization)
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def aes-opt.def
improve_placement
check_placement

set def_file [make_result_file aes-opt.def]
write_def $def_file
diff_file aes-opt.defok $def_file
