# Compare repair after ODB reload with the original in-memory result.
set test_name repair_antennas_odb_roundtrip
source "helpers.tcl"

read_liberty "sky130hs/sky130hs_tt.lib"
read_db [make_result_file "${test_name}.odb"]

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5

check_antennas
repair_antennas
check_antennas
check_placement

set guide_file [make_result_file "${test_name}_cold.guide"]
write_guides $guide_file
if { [diff_files [make_result_file "${test_name}_warm.guide"] $guide_file] } {
  error "Antenna-repaired guides differ after ODB reload."
}

set def_file [make_result_file "${test_name}_cold.def"]
write_def $def_file
if { [diff_files [make_result_file "${test_name}_warm.def"] $def_file] } {
  error "Antenna-repaired DEF differs after ODB reload."
}
