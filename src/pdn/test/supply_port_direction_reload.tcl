# Reload the PDN database in a fresh timing network and compare its exports.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_db [make_result_file supply_port_direction.odb]
create_clock -name clk -period 1.0 [get_ports clk]
foreach {name direction} {VDD power VSS ground} {
  check "$name reloaded direction" { get_property [get_ports $name] direction } $direction
}
write_verilog [make_result_file supply_port_direction_reload.v]
write_timing_model [make_result_file supply_port_direction_reload.lib]
check "Verilog unchanged after read_db" {
  diff_files [make_result_file supply_port_direction.v] \
    [make_result_file supply_port_direction_reload.v]
} 0

foreach stage {supply_port_direction supply_port_direction_reload} {
  set stream [open [make_result_file $stage.lib] r]
  set model [read $stream]
  close $stream
  foreach name {VDD VSS} {
    set pattern [format {pin\("%s"\) \{[^\}]*\}} $name]
    check "$stage has $name" { regexp $pattern $model pin } 1
    check "$stage $name direction" { string match {*direction : input;*} $pin } 1
    dict set supply_pins $stage $name $pin
  }
}
# read_db can change pin order in the Liberty model.
check "Supply pin models unchanged after read_db" {
  string equal [dict get $supply_pins supply_port_direction] \
    [dict get $supply_pins supply_port_direction_reload]
} 1
exit_summary
