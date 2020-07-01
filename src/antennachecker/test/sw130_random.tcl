source "helpers.tcl"
read_lef merged_spacing.lef
read_def -order_wires sw130_random.def

# load layers' antenna rules into ARC
load_antenna_rules

# start checking antennas and generate a detail report
check_antennas

# calculate the available length that can be added to net54, at route level 1, while keeping the PAR ratios satified
get_met_avail_length -net_name "net51" -route_level 1

# check if net52 has a violation
set vio [check_net_violation -net_name "net50"]
puts "this net has violation: $vio"
