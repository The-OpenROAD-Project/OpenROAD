# repair_design max_slew on input port with set_driving_cell
source "helpers.tcl"
read_liberty repair_slew10.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire7.def
set_driving_cell -lib_cell BUF_X1 in1

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

report_check_types -max_slew -max_capacitance

repair_design

report_check_types -max_slew -max_capacitance
# make sure an input buffer was not inserted
report_net in1
