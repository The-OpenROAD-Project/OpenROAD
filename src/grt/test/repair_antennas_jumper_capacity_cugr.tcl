# repair_antennas -jumper_only under CUGR with the jumper layers nearly
# full: candidates whose accumulated wire+via demand overflows an edge are
# rejected (debug log) and insertion retries the remaining candidates.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met3 0.8
set_global_routing_layer_adjustment met4-met5 0.9
set_routing_layers -signal met1-met5
global_route -use_cugr

check_antennas
set_debug_level GRT repair_antennas 1
repair_antennas -jumper_only
check_antennas
check_placement

set guide_file [make_result_file repair_antennas_jumper_capacity_cugr.guide]
write_guides $guide_file
diff_file repair_antennas_jumper_capacity_cugr.guideok $guide_file
