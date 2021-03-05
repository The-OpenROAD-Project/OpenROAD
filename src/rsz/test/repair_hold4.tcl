# repair_timing -hold sky130
source helpers.tcl
read_liberty sky130hs/sky130hs_tt.lib
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def repair_hold4.def

create_clock -period 2 clk
set_propagated_clock clk

set_wire_rc -layer met1
estimate_parasitics -placement

report_checks -path_delay min -format full_clock -digits 3 -group_count 3

repair_timing -hold

report_checks -path_delay min -format full_clock -digits 3 -group_count 3
