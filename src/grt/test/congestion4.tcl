# check route guides for gcd_nangate45. def file from the openroad-flow
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "congestion3.def"

set_global_routing_layer_adjustment metal3 0.9

set_routing_layers -signal metal3-metal3

catch {global_route} error
puts $error
