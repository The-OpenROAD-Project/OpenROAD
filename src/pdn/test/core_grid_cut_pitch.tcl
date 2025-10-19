# test for cut pitch
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -followpins -layer metal2

add_pdn_connect -layers {metal1 metal2} -cut_pitch 0.5

pdngen

set def_file [make_result_file core_grid_cut_pitch.def]
write_def $def_file
diff_files core_grid_cut_pitch.defok $def_file
