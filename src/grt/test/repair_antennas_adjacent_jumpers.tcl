# Regression test for the adjacent-jumper sttree underflow bug.
#
# When two jumpers are inserted on the same net at positions separated by
# exactly one routing tile, updateRouteGridsLayer would promote the right
# endpoint of the first jumper AND the left endpoint of the second jumper
# to the new layer, merging them into a single same-layer chain in the
# sttree. When releaseNetResources later iterated that chain it decremented
# h_edges_3D_ for an edge that was never incremented, wrapping the uint16_t
# usage to 65535. The fix inserts old-layer via-transition points at the
# boundary of each promoted region.
#
# gcd_sky130_long_nets.def is a modified version of gcd_sky130.def with an
# 800 um-wide die and two cells from a high-fanout net moved ~500 um from the
# driver, creating several very long horizontal routes.  Combined with the
# 95 % met2-met5 capacity adjustment below, this forces the router to keep
# long segments on met1 and reliably produces multiple jumpers on the same
# net as well as a remaining violation that needs a diode.  The dirty-net
# reroute triggered by diode insertion calls releaseNetResources on sttrees
# modified by updateRouteGridsLayer, which is the code path that underflowed.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130_long_nets.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met1-met5 0.8
set_routing_layers -signal met1-met5
global_route

check_antennas
repair_antennas
check_antennas
check_placement

set guide_file [make_result_file repair_antennas_adjacent_jumpers.guide]
write_guides $guide_file
diff_file repair_antennas_adjacent_jumpers.guideok $guide_file

set def_file [make_result_file repair_antennas_adjacent_jumpers.def]
write_def $def_file
diff_file repair_antennas_adjacent_jumpers.defok $def_file
