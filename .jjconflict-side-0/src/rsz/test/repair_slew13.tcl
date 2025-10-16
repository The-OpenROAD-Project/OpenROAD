# repair_design impossibly small liberty input pin max_transition (ala fakeram)
# in1--u1---------u2-out1
source "helpers.tcl"
read_liberty sky130hd/sky130hd_tt.lib
read_liberty repair_slew13.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_lef repair_slew13.lef
read_def repair_slew13.def

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement

report_check_types -max_slew
repair_design
report_check_types -max_slew
