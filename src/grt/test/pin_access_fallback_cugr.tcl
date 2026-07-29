# An instance created after pin_access has no preferred access points; its
# pins must fall back to shape-derived APs instead of being dropped from the
# CUGR routing tree (formerly reported as EST-0026 "Missing route to pin").
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_liberty "sky130hs/sky130hs_tt.lib"
read_def "critical_nets_percentage.def"
read_sdc "critical_nets_percentage.sdc"

source "sky130hs/sky130hs.rc"
set_wire_rc -signal -layer "met2"
set_wire_rc -clock -layer "met5"
set_propagated_clock [all_clocks]

set_routing_layers -signal met1-met5 -clock met3-met5
set_global_routing_layer_adjustment met1-met5 0.8

pin_access -verbose 0

# This buffer has no preferred access points, unlike the net's other pins.
set block [ord::get_db_block]
set master [[ord::get_db] findMaster "sky130_fd_sc_hs__buf_4"]
set inst [odb::dbInst_create $block $master "post_pa_buffer"]
$inst setLocation 150000 150000
$inst setPlacementStatus PLACED
[$inst findITerm "A"] connect [$block findNet "_051_"]

estimate_parasitics -placement

global_route -use_cugr -critical_nets_percentage 30 -resistance_aware -verbose

set guide_file [make_result_file pin_access_fallback_cugr.guide]
write_guides $guide_file
diff_file pin_access_fallback_cugr.guideok $guide_file
