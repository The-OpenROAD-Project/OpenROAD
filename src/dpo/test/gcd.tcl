# gcd (low utilization)
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def
improve_placement
check_placement

set def_file [make_result_file gcd.def]
write_def $def_file
diff_file gcd.defok $def_file
