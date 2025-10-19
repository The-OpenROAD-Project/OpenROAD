# test for dual overlappingrings
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths 2.0 -spacings 2.0 -core_offsets 2.0
add_pdn_ring -grid "Core" -layers {metal3 metal4} -widths 2.0 -spacings 2.0 -core_offsets 2.0

add_pdn_stripe -layer metal4 -width 1.0 -pitch 5.0 -offset 2.5 -extend_to_core_ring

add_pdn_connect -layers {metal3 metal4}
add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal1 metal4}

pdngen

set def_file [make_result_file core_grid_with_dual_rings.def]
write_def $def_file
diff_files core_grid_with_dual_rings.defok $def_file
