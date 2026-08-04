# repair_antennas -diode_only with CUGR routing: the first iteration's 11
# diodes leave one violation, so -iterations 2 exercises the repair loop
# re-checking and inserting again after the incremental reroute.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met4 0.8
set_routing_layers -signal met1-met5
global_route -use_cugr

check_antennas
repair_antennas -diode_only -iterations 2
check_antennas
check_placement

set guide_file [make_result_file repair_antennas_only_diodes_cugr.guide]
write_guides $guide_file
diff_file repair_antennas_only_diodes_cugr.guideok $guide_file

set def_file [make_result_file repair_antennas_only_diodes_cugr.def]
write_def $def_file
diff_file repair_antennas_only_diodes_cugr.defok $def_file
