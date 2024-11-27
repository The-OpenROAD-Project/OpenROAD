# test for rounding on M6 for min area
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

# Adjust techrules to set min area on metal6
[[ord::get_db_tech] findLayer metal6] setArea 0.5898

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core" -pins metal7
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -followpins -layer metal2

add_pdn_stripe -layer metal4 -width 0.48 -pitch 15.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.40 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal2}
add_pdn_connect -layers {metal2 metal4}
add_pdn_connect -layers {metal4 metal7}

pdngen

set def_file [make_result_file core_grid_with_M6_min_area.def]
write_def $def_file
diff_files core_grid_with_M6_min_area.defok $def_file
