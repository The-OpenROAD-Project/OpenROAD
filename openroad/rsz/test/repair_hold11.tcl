# repair_timing -hold 2 corners
source helpers.tcl
read_liberty Nangate45/Nangate45_slow.lib
read_liberty Nangate45/Nangate45_fast.lib
define_scene fast -liberty Nangate45/Nangate45_fast.lib
define_scene slow -liberty Nangate45/Nangate45_slow.lib
set_scene fast
read_lef Nangate45/Nangate45.lef
read_def repair_hold1.def

create_clock -period 2 clk
set_input_delay -clock clk 0 {in1 in2}
set_output_delay -clock clk -0.1 out
set_propagated_clock clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_worst_slack -min
report_worst_slack -max

repair_timing -hold

report_worst_slack -min
report_worst_slack -max
