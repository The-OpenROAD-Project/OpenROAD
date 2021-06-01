# repair_timing -hold -margin
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold1.def

create_clock -period 2 clk
set_input_delay -clock clk 0 {in1 in2}
set_output_delay -clock clk -0.1 out
set_propagated_clock clk

set_wire_rc -layer metal1
estimate_parasitics -placement

report_worst_slack -min
report_worst_slack -max

repair_timing -hold -slack_margin .2

report_worst_slack -min
report_worst_slack -max
