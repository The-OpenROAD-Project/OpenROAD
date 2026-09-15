# Antenna repair must produce the same guides and DEF before and after ODB reload.
set test_name repair_antennas_odb_roundtrip
source "helpers.tcl"

read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5
global_route

# Save unrepaired guides for repair in a fresh process.
write_db [make_result_file "${test_name}.odb"]

check_antennas
repair_antennas
check_antennas
check_placement
write_guides [make_result_file "${test_name}_warm.guide"]
write_def [make_result_file "${test_name}_warm.def"]

# Reload in a separate process to avoid reusing in-memory routes.
puts [exec [info nameofexecutable] -no_splash -no_init -exit \
  [file join [file dirname [info script]] "${test_name}_reload.tcl"] 2>@1]
