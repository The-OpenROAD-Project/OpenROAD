# CUGR ranks via candidates by priority (OR_DEFAULT, then fewest cuts, then
# LEF-default, then smallest enclosure) rather than db declaration order.
# via_priority.lef offers three metal1/metal2 vias, none OR_DEFAULT: VIA_BIG
# (declared first, large), VIA_SMALL (single-cut, small), and VIA_MULTI (LEF
# DEFAULT but two cuts, even smaller). The model must pick VIA_SMALL -- proving
# db order does not win and that single-cut beats a LEF-default multi-cut via.
# The via_geom debug line pins the chosen via and its enclosure.
source "helpers.tcl"
read_lef "via_priority.lef"
read_def "via_priority.def"

set_routing_layers -signal metal1-metal2
set_debug_level GRT via_geom 1

global_route -use_cugr
