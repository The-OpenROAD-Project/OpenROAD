# ibex (low utilization)
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def ibex-opt.def
improve_placement
check_placement

set def_file [make_result_file ibex-opt.def]
write_def $def_file
diff_file $def_file ibex-opt.defok
