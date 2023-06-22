# repair_hold input fanout hold violation, other setup violation
source helpers.tcl
read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_hold13.def

source sky130hd/sky130hd.vars
source sky130hd/sky130hd.rc
set_wire_rc -layer met2

set_dont_use sky130_fd_sc_hd__dly*

create_clock -period 2 clk
set_propagated_clock clk
set_input_delay -clock clk -1 in1
set_load .2 u1x
set_load .2 u2x
set_load .2 u3x
set_load .2 u4x

estimate_parasitics -placement
report_worst_slack -min
report_worst_slack -max

repair_timing -hold

report_checks -path_delay min

set_clock_uncertainty 0.7 clk
unset_dont_use sky130_fd_sc_hd__dly*

repair_timing -hold

report_checks -path_delay min

