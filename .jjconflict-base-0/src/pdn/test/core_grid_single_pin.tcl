# test for adding top level pins on M7 with a single via connected
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core" -pins metal7
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -followpins -layer metal2

add_pdn_stripe -layer metal4 -width 0.48 -pitch 80.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.40 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal2}
add_pdn_connect -layers {metal2 metal4}
add_pdn_connect -layers {metal4 metal7}

pdngen

set def_file [make_result_file core_grid_single_pin.def]
write_def $def_file
diff_files core_grid_single_pin.defok $def_file
