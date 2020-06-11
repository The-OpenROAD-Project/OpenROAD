# repair_tie_fanout to memory and pad
source "helpers.tcl"
source "tie_fanout.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x32.lib
read_liberty pad.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x32.lef
read_lef pad.lef
read_def repair_tie_fanout5.def

repair_tie_fanout -separation 5 LOGIC1_X1/Z
check_ties LOGIC1_X1
report_ties

set def_file [make_result_file repair_tie_fanout5.def]
write_def $def_file
#diff_files rebuffer1.defok $def_file
