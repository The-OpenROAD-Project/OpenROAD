# Report the rejected macro connection while keeping both supply nets connected.
source "helpers.tcl"
set test_name generated_via_default_edge

read_lef Nangate45/Nangate45.lef
read_lef generated_via_enclosure/edge_macro.lef
read_def generated_via_enclosure/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
global_connect
set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name core -pins {metal7} -starts_with POWER
add_pdn_stripe -grid core -layer metal6 -width 0.93 -pitch 10 -offset 2
add_pdn_stripe -grid core -layer metal7 -width 1.4 -pitch 10 -offset 2
add_pdn_connect -grid core -layers {metal6 metal7}

define_pdn_grid -name macro_grid -macro -cells {enclosure_macro} \
  -grid_over_boundary -starts_with POWER
add_pdn_stripe -grid macro_grid -layer metal5 -width 0.93 -pitch 10 -offset 2
add_pdn_stripe -grid macro_grid -layer metal6 -width 0.93 -pitch 10 -offset 2
add_pdn_connect -grid macro_grid -layers {metal4 metal5}
add_pdn_connect -grid macro_grid -layers {metal5 metal6}
add_pdn_connect -grid macro_grid -layers {metal6 metal7}

set rpt_file [make_result_file ${test_name}.rpt]
pdngen -failed_via_report $rpt_file
diff_files ${test_name}.rptok $rpt_file

set inst [[ord::get_db_block] findInst macro]
if { [$inst getOrient] ne "R0" || [$inst getLocation] ne "40000 40000" } {
  error "Macro pose changed"
}

check_power_grid -net VDD
check_power_grid -net VSS
