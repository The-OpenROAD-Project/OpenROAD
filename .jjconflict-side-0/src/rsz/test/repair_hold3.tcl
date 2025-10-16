# repair_timing -hold fanout regs with and without hold violations from clk skew
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold3.def

create_clock -period 2 clk
set_propagated_clock clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_worst_slack -min
report_worst_slack -max

write_verilog_for_eqy repair_hold3 before "None"
repair_timing -hold
run_equivalence_test repair_hold3 ./Nangate45/work_around_yosys/ "None"

report_worst_slack -min
report_worst_slack -max
