# test for connecting connecting followpins on both sides
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal2 -width 1.0 -pitch 10.0 -extend_to_boundary
add_pdn_connect -layers {metal1 metal2}

pdngen

set def_file [make_result_file core_grid_with_m2_straps.def]
write_def $def_file
diff_files core_grid_with_m2_straps.defok $def_file
