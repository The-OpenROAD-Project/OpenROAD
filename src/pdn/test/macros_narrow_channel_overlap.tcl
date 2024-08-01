# test for repair channel with a narrow channel with a partially blocked channel
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan_narrow.def

# Create via obstruction
odb::dbObstruction_create [ord::get_db_block] [[ord::get_db_tech] findLayer metal4] 76000 99000 80000 210000

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal6 -width 10.0 -spacing 4.0 -pitch 65.0 -offset 18.0
add_pdn_stripe -layer metal7 -width 1.4 -pitch 40.0 -offset 2.0

add_pdn_connect -layers {metal1 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file macros_narrow_channel_overlap.def]
write_def $def_file
diff_files macros_narrow_channel_overlap.defok $def_file
