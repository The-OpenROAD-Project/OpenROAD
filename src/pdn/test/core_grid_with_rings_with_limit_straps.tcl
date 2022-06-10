# test for grid with secondardy nets and limiting straps to only two nets
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VDD2 -pin_pattern VDD2 -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS -secondary_power VDD2

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

add_pdn_stripe -layer metal4 -width 1.0 -pitch 5.0 -offset 2.5 -extend_to_core_ring -nets "VDD VSS"
add_pdn_stripe -layer metal5 -width 1.0 -pitch 5.0 -offset 2.5 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths 2.0 -spacings 2.0 -core_offsets 2.0

add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal1 metal6}
add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal5}

pdngen

set def_file [make_result_file core_grid_with_rings_with_limit_straps.def]
write_def $def_file
diff_files core_grid_with_rings_with_limit_straps.defok $def_file
