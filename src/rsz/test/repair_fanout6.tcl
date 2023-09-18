# fanout 8000 max_fanout 20 stress test
# modified to use large default max_transition, max_capacitance
source "helpers.tcl"
read_liberty repair_fanout6.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_fanout6.def

create_clock -period 5 clk
set_max_fanout 20 [current_design]

source sky130hd/sky130hd.vars
source sky130hd/sky130hd.rc
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock  -layer $wire_rc_layer_clk
set_dont_use $dont_use

estimate_parasitics -placement

repair_design -max_wire_length 100000

report_worst_slack -max

report_check_types -max_fanout

# It is possible to get better timing resuilts with repair_design 
# but there is no point in inserting extra buffers to fix non critical
# paths. What matters is repair_timning's ability to optimize the timing
# when it matters.
repair_timing -setup -repair_tns 0 -skip_gate_cloning 
report_worst_slack -max
