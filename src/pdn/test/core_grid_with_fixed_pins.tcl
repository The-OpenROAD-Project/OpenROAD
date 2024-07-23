# test for grid with fixed pins on boundary
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

add_pdn_stripe -grid "Core" -layer metal4 -width 2.0 -pitch 5.0 -offset 1.0 -extend_to_boundary

add_pdn_connect -layers {metal1 metal4}

pdngen

set def_file [make_result_file core_grid_with_fixed_pins.def]
write_def $def_file
diff_files core_grid_with_fixed_pins.defok $def_file
