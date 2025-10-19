# repair_slew15 with alpha=0 (should give the same results)
source "helpers.tcl"
read_liberty repair_slew10.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew10.def
set_max_transition 0.08 [current_design]

# 10x wire resistance
set_wire_rc -resistance 3.574e-02 -capacitance 7.516e-02
estimate_parasitics -placement

set_routing_alpha 0

puts "Found [sta::max_slew_violation_count] slew violations"
repair_design
puts "Found [sta::max_slew_violation_count] slew violations"
