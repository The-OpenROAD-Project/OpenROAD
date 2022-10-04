# repair_slew10 with set_max_transition design
source "helpers.tcl"
read_liberty repair_slew10.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew10.def
set_max_transition 0.08 [current_design]

# 10x wire resistance
set_wire_rc -resistance 3.574e-02 -capacitance 7.516e-02
estimate_parasitics -placement

report_check_types -max_slew -max_cap
repair_design
report_check_types -max_slew -max_cap
