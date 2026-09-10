# read segments file where a disconnected segment is masked by
# multi-fanout junctions in the same net
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty Nangate45/Nangate45_typ.lib
read_def "gcd.def"

set_routing_layers -signal metal1-metal10
catch { read_global_route_segments read_segments_error6.segs } error
puts $error
