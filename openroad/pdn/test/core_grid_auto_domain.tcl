# test for automatically creating the core voltage domain
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

pdngen

set def_file [make_result_file core_grid_auto_domain.def]
write_def $def_file
diff_files core_grid.defok $def_file
