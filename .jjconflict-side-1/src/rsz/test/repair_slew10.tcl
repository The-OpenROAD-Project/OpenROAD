# repair_design max slew, wire RC -> load slew violation, wire under max length
# BUF_X1 cell with max_transition on input
source "helpers.tcl"
read_liberty repair_slew10.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew10.def

# 10x wire resistance
set_wire_rc -resistance 3.574e-02 -capacitance 7.516e-02
estimate_parasitics -placement

report_check_types -max_slew -max_cap
repair_design
report_check_types -max_slew -max_cap
