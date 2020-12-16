# repair_timing -hold -max_utilization
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def
create_clock [get_ports clk] -name core_clock -period 2
# forced large hold violations
set_clock_uncertainty -hold .4 core_clock

set_wire_rc -layer metal3
estimate_parasitics -placement

set_dont_use {CLKBUF_* AOI211_X1 OAI211_X1}

report_design_area
catch {repair_timing -hold -max_utilization 13} error
report_design_area
