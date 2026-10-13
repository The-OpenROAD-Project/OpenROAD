# Followpin rails must not depend on whether the rows are mirrored in X.
# MY ("FN") and R0 ("N") both leave the power rail at the top of the row;
# only MX ("FS") and R180 ("S") move it to the bottom.  This floorplan uses
# the FS/N/S/FN cycle that a double-patterned PDK asks for, so its power
# geometry is identical to a plain FS/N floorplan and the generated rails must
# alternate VDD/VSS on every row boundary.
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_mirrored_rows/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground
global_connect

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -pitch 5.0 -offset 2.5
add_pdn_connect -layers {metal1 metal4}

pdngen

set def_file [make_result_file core_grid_mirrored_rows.def]
write_def $def_file
diff_files core_grid_mirrored_rows.defok $def_file
