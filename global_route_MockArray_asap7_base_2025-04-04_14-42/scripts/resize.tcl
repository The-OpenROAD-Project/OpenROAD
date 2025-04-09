utl::set_metrics_stage "placeopt__{}"
source $::env(SCRIPTS_DIR)/load.tcl
erase_non_stage_variables place
load_design 3_3_place_gp.odb 2_floorplan.sdc

estimate_parasitics -placement

set instance_count_before [sta::network_leaf_instance_count]
set pin_count_before [sta::network_leaf_pin_count]

set_dont_use $::env(DONT_USE_CELLS)

# Do not buffer chip-level designs
# by default, IO ports will be buffered
# to not buffer IO ports, set environment variable
# DONT_BUFFER_PORT = 1
if { ![env_var_exists_and_non_empty FOOTPRINT] } {
  if { ![env_var_equals DONT_BUFFER_PORTS 1] } {
    puts "Perform port buffering..."
    buffer_ports
  }
}

repair_design_helper

if { [env_var_exists_and_non_empty TIE_SEPARATION] } {
  set tie_separation $env(TIE_SEPARATION)
} else {
  set tie_separation 0
}

# Repair tie lo fanout
puts "Repair tie lo fanout..."
set tielo_cell_name [lindex $env(TIELO_CELL_AND_PORT) 0]
set tielo_lib_name [get_name [get_property [lindex [get_lib_cell $tielo_cell_name] 0] library]]
set tielo_pin $tielo_lib_name/$tielo_cell_name/[lindex $env(TIELO_CELL_AND_PORT) 1]
repair_tie_fanout -separation $tie_separation $tielo_pin

# Repair tie hi fanout
puts "Repair tie hi fanout..."
set tiehi_cell_name [lindex $env(TIEHI_CELL_AND_PORT) 0]
set tiehi_lib_name [get_name [get_property [lindex [get_lib_cell $tiehi_cell_name] 0] library]]
set tiehi_pin $tiehi_lib_name/$tiehi_cell_name/[lindex $env(TIEHI_CELL_AND_PORT) 1]
repair_tie_fanout -separation $tie_separation $tiehi_pin

# hold violations are not repaired until after CTS

# post report

puts "Floating nets: "
report_floating_nets

report_metrics 3 "resizer" true false

puts "Instance count before $instance_count_before, after [sta::network_leaf_instance_count]"
puts "Pin count before $pin_count_before, after [sta::network_leaf_pin_count]"

write_db $::env(RESULTS_DIR)/3_4_place_resized.odb
