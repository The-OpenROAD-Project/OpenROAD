# repair_design max_fanout with set_driving_cell
source "helpers.tcl"
# nangate45_typ BUF_X1 with max_fanout attribute
read_liberty repair_fanout4.lib
read_lef Nangate45/Nangate45.lef
read_def repair_fanout4.def
set_driving_cell -lib_cell BUF_X1 in1

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_check_types -max_fanout
repair_design
report_check_types -max_fanout
