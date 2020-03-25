# gcd (low utilization)
source helpers.tcl
read_lef Nangate45.lef
read_def gcd_replace.def
detailed_placement
check_placement

set def_file [make_result_file gcd.def]
write_def $def_file
diff_file gcd.defok $def_file
