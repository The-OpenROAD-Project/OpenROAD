# fanout 8000 max_fanout 20 stress test
# modified to use large default max_transition, max_capacitance
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

report_checks -path_delay max -fields {slew cap input nets fanout} -format full_clock_expanded

report_check_types -max_fanout
