# check route guides for gcd_nangate45. def file from the openroad-flow
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set_global_routing_layer_adjustment metal2 0.9
set_global_routing_layer_adjustment metal3 0.9
set_global_routing_layer_adjustment metal4-metal10 0.8

set_routing_layers -signal metal2-metal10

catch {global_route} error
puts $error
