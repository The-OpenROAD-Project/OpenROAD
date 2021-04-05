# repair_timing -hold min/max delay prevents complete repair
source helpers.tcl
read_liberty sky130hs/sky130hs_tt.lib
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def repair_hold4.def

create_clock -period 2 clk
set_propagated_clock clk
set_max_delay -ignore_clock_latency -from r1/CLK -to r3/D 0.2

set_wire_rc -layer met1
estimate_parasitics -placement

report_checks -path_delay min -format full_clock -digits 3 -to r2/D
report_checks -path_delay min_max -format full_clock -digits 3 -to r3/D
report_checks -path_delay min -format full_clock -digits 3 -to r4/D

repair_timing -hold

report_checks -path_delay min -format full_clock -digits 3 -to r2/D
# fails setup and hold (cannot fix without -allow_setup_violations)
report_checks -path_delay min_max -format full_clock -digits 3 -to r3/D
report_checks -path_delay min -format full_clock -digits 3 -to r4/D
