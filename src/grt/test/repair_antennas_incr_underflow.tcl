# Regression for OpenROAD issue #10273.
#
# After antenna repair inserts diodes/jumpers, the modified nets are rerouted
# incrementally with the 3D maze router (mazeRouteMSMDOrder3D). If the source
# subtree of an edge cannot reach the destination subtree within the
# constrained reroute region, the maze search exhausts its priority queue.
# Previously this aborted the whole flow with:
#   [ERROR GRT-0183] Net <name>: heap underflow during 3D maze routing.
#
# The router now keeps the edge's original route (recoverEdge) and continues
# instead of aborting. This test exercises the diode-insertion incremental
# reroute path and checks that the flow completes successfully.
#
# NOTE: The original heap-underflow trigger is specific to the ihp130 sg13g2
# tinyRocket design (external test case, not in this repo), so this test does
# not by itself force the underflow on the available sky130 designs; it guards
# the antenna-repair incremental reroute code path against regressions.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5
global_route

check_antennas
# diode-only repair forces the IncrementalGRoute + updateRoutes path that
# reroutes repaired nets with the 3D maze router.
repair_antennas -diode_only
check_antennas
check_placement

puts "repair_antennas incremental reroute completed without GRT-0183"
