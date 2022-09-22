# buffer chain with set_max_delay
source "helpers.tcl"
read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_setup5.def

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement

set_max_delay -from u1/X -to u5/A 2
report_worst_slack -max
repair_timing -setup
report_worst_slack -max
