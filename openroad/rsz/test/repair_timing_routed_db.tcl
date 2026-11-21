# repair_timing on a detail-routed design keeps the parasitics the flow
# estimated instead of switching to detailed_routing parasitics without
# usable global routes (issue #11852, RSZ-0074).
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_routed.def

# A period tight enough for setup violations.
create_clock [get_ports clk] -name core_clock -period 0.43

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "design_is_routed: [design_is_routed]"
repair_timing -setup
puts "parasitics source: [est::parasitics_source]"
report_worst_slack -max -digits 3
