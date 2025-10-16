# test for handling bterms on edges with no connectivity
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

initialize_floorplan -die_area [ord::get_die_area] \
  -core_area [ord::get_die_area] \
  -site FreePDK45_38x28_10R_NP_162NW_34O

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 -offset 2.0 -extend_to_boundary
add_pdn_stripe -layer metal7 -width 1.40 -pitch 20.0 -offset 2.0 -extend_to_boundary
add_pdn_stripe -layer metal8 -width 1.40 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}
add_pdn_connect -layers {metal7 metal8}

# Block metal7
set layer [[ord::get_db_tech] findLayer metal7]
odb::dbObstruction_create [ord::get_db_block] $layer 161000 21000 198000 186000

pdngen

set def_file [make_result_file core_grid_with_single_edge_pins.def]
write_def $def_file
diff_files core_grid_with_single_edge_pins.defok $def_file
