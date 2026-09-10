# CUGR resistance-aware routing on a (synthetic) high-resistance layer.
#
# met1 is given 10x its normal per-unit resistance via set_layer_rc, which
# updates both the routing cost (dbTechLayer) and timing extraction. With a
# relaxed (uncongested) adjustment of 0.3 the only pressure to leave met1 is
# the resistance penalty, so this isolates the resistance response: critical
# nets migrate off the high-resistance met1 (usage ~1225 -> ~891) onto the
# lower-resistance met3 (~46 -> ~415). The guides golden captures that
# redistribution (see GRT-0096 usage in the .ok).
#
# Mechanism-validation test (deterministic). Timing is NOT asserted here:
# gcd is cell-delay-limited, so layer choice does not drive its WNS/TNS
# (a 10x met1 moves OFF TNS by only ~1 ns). Timing validation of
# resistance-aware belongs at ORFS scale on wire-resistance-limited designs.
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_liberty "sky130hs/sky130hs_tt.lib"
read_def "critical_nets_percentage.def"
read_sdc "critical_nets_percentage.sdc"

source "sky130hs/sky130hs.rc"
set_wire_rc -signal -layer "met2"
set_wire_rc -clock -layer "met5"

# Stress the most-used layer (met1) with 10x its normal resistance.
set_layer_rc -layer met1 -resistance 8.929e-3 -capacitance 1.72375E-04

set_propagated_clock [all_clocks]
estimate_parasitics -placement

set guide_file [make_result_file resistance_aware_highres_cugr.guide]

set_routing_layers -signal met1-met5 -clock met3-met5
set_global_routing_layer_adjustment met1-met5 0.3

global_route -use_cugr -critical_nets_percentage 30 -resistance_aware -verbose

write_guides $guide_file

diff_file resistance_aware_highres_cugr.guideok $guide_file
