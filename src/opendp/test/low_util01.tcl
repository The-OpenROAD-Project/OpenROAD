# gcd (low utilization)
source helpers.tcl
read_lef Nangate45.lef
read_def nangate45-bench/gcd/gcd_replace.def
detailed_placement
check_placement

set def_file [make_result_file low_util01.def]
write_def $def_file
diff_file $def_file low_util01.defok
