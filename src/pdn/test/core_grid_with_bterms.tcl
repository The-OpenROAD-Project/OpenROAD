# test for connecting to fixed bterms
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set term [odb::dbBTerm_create [[ord::get_db_block] findNet VDD] "VDD"]

# Create BTerm on metal2
# This one should be included in grid
set pin [odb::dbBPin_create $term]
odb::dbBox_create $pin [[ord::get_db_tech] findLayer metal2] \
  [ord::microns_to_dbu 24.75] [ord::microns_to_dbu 0] \
  [ord::microns_to_dbu 25.25] [ord::microns_to_dbu 0.5]
$pin setPlacementStatus FIRM

# This one is larger than the shape so reject
set pin [odb::dbBPin_create $term]
odb::dbBox_create $pin [[ord::get_db_tech] findLayer metal2] \
  [ord::microns_to_dbu 34.00] [ord::microns_to_dbu 0] \
  [ord::microns_to_dbu 35.00] [ord::microns_to_dbu 0.5]
$pin setPlacementStatus FIRM

# Create BTerm on metal3
# Connect to this one
set pin [odb::dbBPin_create $term]
odb::dbBox_create $pin [[ord::get_db_tech] findLayer metal3] \
  [ord::microns_to_dbu 14.75] [ord::microns_to_dbu 0] \
  [ord::microns_to_dbu 15.25] [ord::microns_to_dbu 0.5]
$pin setPlacementStatus FIRM

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal2 -width 1.0 -pitch 10.0 -extend_to_boundary
add_pdn_connect -layers {metal1 metal2}
add_pdn_connect -layers {metal2 metal3}

pdngen

set def_file [make_result_file core_grid_with_bterms.def]
write_def $def_file
diff_files core_grid_with_bterms.defok $def_file
