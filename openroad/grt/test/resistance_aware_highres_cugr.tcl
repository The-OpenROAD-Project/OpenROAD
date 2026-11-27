# CUGR delay cost on a (synthetic) high-resistance layer.
#
# met1 is given 10x its normal per-unit resistance via set_layer_rc, which
# the delay cost and timing extraction both read. With a relaxed
# (uncongested) adjustment of 0.3 the only pressure to leave met1 is the
# delay cost, so this isolates the resistance response: critical nets move
# off met1 onto lower-resistance layers. The guides golden captures that
# redistribution (see the congestion report in the .ok).
#
# Mechanism-validation test (deterministic). Timing is NOT asserted here:
# gcd is cell-delay-limited, so layer choice barely moves its WNS/TNS.
# Timing validation belongs at ORFS scale on wire-resistance-limited designs.
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

global_route -use_cugr -verbose

write_guides $guide_file

diff_files resistance_aware_highres_cugr.guideok $guide_file
