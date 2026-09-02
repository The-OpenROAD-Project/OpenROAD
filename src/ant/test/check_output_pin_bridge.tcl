source "helpers.tcl"
# Two disjoint li1 stubs bridged only by the driver output pin u1/X: the
# bridged metal is one node, so both input gates must see the full record
read_lef ant_check.lef
read_def check_output_pin_bridge.def

check_antennas -verbose
puts "violation count = [ant::antenna_violation_count]"

set net "n1"
puts "Net $net violations: [ant::check_net_violation $net]"
