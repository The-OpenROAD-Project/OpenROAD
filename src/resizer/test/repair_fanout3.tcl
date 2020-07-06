# repair_design max_fanout non-liberty loads
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_lef repair_fanout3.lef
read_def repair_fanout3.def

set_max_fanout 10 [current_design]
report_check_types -max_fanout
repair_design -buffer_cell BUF_X1
report_check_types -max_fanout
