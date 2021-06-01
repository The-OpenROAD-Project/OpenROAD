# repair_timing -hold 2 corners
source helpers.tcl
define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
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

repair_timing -hold

report_worst_slack -min
report_worst_slack -max
