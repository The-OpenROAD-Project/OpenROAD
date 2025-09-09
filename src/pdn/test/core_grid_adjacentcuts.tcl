# test for v54 adjacent cut spacing rule
source "helpers.tcl"

read_lef Nangate45_vias/Nangate45_adjacentcuts.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

add_pdn_stripe -layer metal4 -width 2.0 -pitch 10.0 -offset 2.5 -extend_to_core_ring
add_pdn_stripe -layer metal5 -width 2.0 -pitch 10.0 -offset 2.5 -extend_to_core_ring

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal5}

pdngen

set def_file [make_result_file core_grid_adjacentcuts.def]
write_def $def_file
diff_files core_grid_adjacentcuts.defok $def_file
