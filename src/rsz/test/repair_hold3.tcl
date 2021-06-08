# repair_timing -hold fanout regs with and without hold violations from clk skew
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold3.def

create_clock -period 2 clk
set_propagated_clock clk

set_wire_rc -layer metal1
estimate_parasitics -placement

report_worst_slack -min
report_worst_slack -max

repair_timing -hold

report_worst_slack -min
report_worst_slack -max
