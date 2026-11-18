# repair_design max_fanout with set_max_fanout with input fanout > max_fanout
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_fanout4.def
set_max_fanout 10 [current_design]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_check_types -max_fanout
repair_design
report_check_types -max_fanout
