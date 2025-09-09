# repair_timing -hold repair_hold5 with -allow_setup_violations
source helpers.tcl
read_liberty sky130hs/sky130hs_tt.lib
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def repair_hold4.def

create_clock -period 2 clk
set_propagated_clock clk

source sky130hs/sky130hs.rc
set_wire_rc -layer met2
estimate_parasitics -placement

# fails hold
report_slack r2/D
# fails setup and hold
report_slack r3/D
# no violations
report_slack r4/D

repair_timing -hold -allow_setup_violations

report_slack r2/D
report_slack r3/D
report_slack r4/D
