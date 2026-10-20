# PDN supply port directions and exports must agree before and after read_db.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_gcd/floorplan.def
create_clock -name clk -period 1.0 [get_ports clk]

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground
set_voltage_domain -power VDD -ground VSS
define_pdn_grid -name Core -pins metal2
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal2 -width 1.0 -pitch 10.0 -extend_to_boundary
add_pdn_connect -layers {metal1 metal2}

# Build the timing graph before the signal types change, then rebuild it.
report_checks
pdngen
report_checks
foreach {name direction} {VDD power VSS ground} {
  check "$name direction" { get_property [get_ports $name] direction } $direction
}
write_verilog [make_result_file supply_port_direction.v]
write_timing_model [make_result_file supply_port_direction.lib]
write_db [make_result_file supply_port_direction.odb]

puts [exec [info nameofexecutable] -no_splash -no_init -exit \
  -threads [thread_count] \
  [file join [file dirname [info script]] supply_port_direction_reload.tcl] 2>@1]
exit_summary
