# test for error checking against the manufacturing grid
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_gcd/floorplan.def

odb::dbTrackGrid_destroy [[ord::get_db_block] findTrackGrid [[ord::get_db_tech] findLayer "metal1"]]

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
catch { add_pdn_stripe -layer metal1 -width 1.0 -pitch 4 -snap_to_grid } err
puts $err
