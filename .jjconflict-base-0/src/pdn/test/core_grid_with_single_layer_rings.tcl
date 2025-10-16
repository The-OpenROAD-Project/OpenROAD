# test for grid with ring
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {metal6} -widths 2.0 -spacings 2.0 -core_offsets 2.0

add_pdn_connect -layers {metal1 metal6}

pdngen

set def_file [make_result_file core_grid_with_single_layer_rings.def]
write_def $def_file
diff_files core_grid_with_single_layer_rings.defok $def_file
