# check route guides for gcd_nangate45. def file from the openroad-flow
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

catch {set_routing_layers -signal met1-met5 -clock met3-met5} error
puts $error
