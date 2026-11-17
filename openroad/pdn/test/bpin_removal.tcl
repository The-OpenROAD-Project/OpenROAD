# test for removing non-fixed bterms
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

# Create bterms
set net [[ord::get_db_block] findNet VDD]
set bterm [odb::dbBTerm_create $net "VDD"]
set bpin0 [odb::dbBPin_create $bterm]
odb::dbBox_create $bpin0 [[ord::get_db_tech] findLayer metal2] 0 200 100 300
$bpin0 setPlacementStatus PLACED

set bpin1 [odb::dbBPin_create $bterm]
odb::dbBox_create $bpin1 [[ord::get_db_tech] findLayer metal3] 0 200 100 300
$bpin1 setPlacementStatus LOCKED

pdngen

set def_file [make_result_file bpin_removal.def]
write_def $def_file
diff_files bpin_removal.defok $def_file
