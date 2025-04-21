# gcd (low utilization)
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd-opt.def
improve_placement
check_placement

set def_file [make_result_file gcd-opt.def]
write_def $def_file
diff_file gcd-opt.defok $def_file
