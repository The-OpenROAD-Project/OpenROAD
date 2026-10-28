# report_congestion and has_congestion with FastRoute.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

# Both commands need a global routing first.
catch { has_congestion }

global_route
puts "has_congestion: [has_congestion]"

# Tight capacity so congestion remains.
set_global_routing_layer_adjustment metal2 0.9
set_global_routing_layer_adjustment metal3 0.9
set_global_routing_layer_adjustment metal4-metal10 1
global_route -allow_congestion
puts "has_congestion: [has_congestion]"
if { [has_congestion] } {
  report_congestion
}
