# repair_antennas using gcd_grt_cugr.db (CUGR global_route already executed);
# the engine choice is restored from the "grt_use_cugr" block property, shown
# by the CUGR-only "Demand adoption" debug tally and grt::is_use_cugr.
# gcd_grt_cugr.db generation: read sky130hs libs + gcd_sky130.def,
#   set_global_routing_layer_adjustment met2-met3 0.15
#   set_routing_layers -signal met1-met3
#   global_route -use_cugr
#   write_db gcd_grt_cugr.db
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_db "gcd_grt_cugr.db"

set_debug_level GRT repair_antennas 1

check_antennas
repair_antennas
puts "use_cugr: [grt::is_use_cugr]"
check_antennas
check_placement

set guide_file [make_result_file repair_antennas_from_odb_cugr.guide]
write_guides $guide_file
diff_file repair_antennas_from_odb_cugr.guideok $guide_file

set def_file [make_result_file repair_antennas_from_odb_cugr.def]
write_def $def_file
diff_file repair_antennas_from_odb_cugr.defok $def_file
