# repair_timing -hold fanout regs with and without hold violations from clk skew
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold3.def

create_clock -period 2 clk
set_propagated_clock clk

set_wire_rc -layer metal1
estimate_parasitics -placement

report_checks -path_delay min -format full_clock -digits 3 -group_count 3

repair_timing -hold

report_checks -path_delay min -format full_clock -digits 3 -group_count 3
