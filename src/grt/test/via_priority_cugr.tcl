# CUGR ranks via candidates by priority (OR_DEFAULT, then LEF-default, then
# fewest cuts, then smallest enclosure) rather than db declaration order.
# via_priority.lef declares the larger VIA_BIG before VIA_SMALL for the
# metal1/metal2 pair, neither marked OR_DEFAULT; the via-demand model must pick
# VIA_SMALL. The via_geom debug line pins the chosen via and its enclosure.
source "helpers.tcl"
read_lef "via_priority.lef"
read_def "via_priority.def"

set_routing_layers -signal metal1-metal2
set_debug_level GRT via_geom 1

global_route -use_cugr
