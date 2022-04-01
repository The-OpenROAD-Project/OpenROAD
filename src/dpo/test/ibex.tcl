# ibex (low utilization)
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def ibex.def
improve_placement
check_placement

set def_file [make_result_file ibex.def]
write_def $def_file
diff_file $def_file ibex.defok
