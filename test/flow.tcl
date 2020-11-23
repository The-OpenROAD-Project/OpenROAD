# assumes flow_helpers.tcl has been read
read_libraries
read_verilog $synth_verilog
link_design $top_module
read_sdc $sdc_file

initialize_floorplan -site $site \
  -die_area $die_area \
  -core_area $core_area \
  -tracks $tracks_file

# remove buffers inserted by synthesis 
remove_buffers

################################################################
# IO Placement
io_placer -random -hor_layer $io_placer_hor_layer -ver_layer $io_placer_ver_layer

################################################################
# Macro Placement
if { [have_macros] } {
  # tdms_place (but replace isn't timing driven)
  global_placement -disable_routability_driven -density $global_place_density

  macro_placement -global_config $ip_global_cfg
}

################################################################
# Tapcell insertion
eval tapcell $tapcell_args

################################################################
# Power distribution network insertion
pdngen -verbose $pdn_cfg

################################################################
# Global placement
global_placement -disable_routability_driven \
  -density $global_place_density \
  -init_density_penalty $global_place_density_penalty \
  -pad_left $global_place_pad -pad_right $global_place_pad

# checkpoint
set global_place_def [make_result_file ${design}_${platform}_global_place.def]
write_def $global_place_def

################################################################
# Resize
# estimate wire rc parasitics
set_wire_rc -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
estimate_parasitics -placement
set_dont_use $dont_use

repair_design -max_wire_length $max_wire_length \
  -buffer_cell $resize_buffer_cell

repair_tie_fanout -separation $tie_separation $tielo_port
repair_tie_fanout -separation $tie_separation $tiehi_port

set_placement_padding -global -left $detail_place_pad -right $detail_place_pad
detailed_placement
optimize_mirroring
check_placement -verbose

# post resize timing report (ideal clocks)
report_checks -path_delay min_max -format full_clock_expanded \
  -fields {input_pin slew capacitance} -digits 3
report_check_types -max_slew -max_capacitance -max_fanout -violators

################################################################
# Clock Tree Synthesis
# Clone clock tree inverters next to register loads
# so cts does not try to buffer the inverted clocks.
repair_clock_inverters

clock_tree_synthesis -lut_file $cts_lut_file \
  -sol_list $cts_sol_file \
  -root_buf $cts_buffer \
  -wire_unit 20

# CTS leaves a long wire from the pad to the clock tree root.
repair_clock_nets -max_wire_length $max_wire_length \
  -buffer_cell $resize_buffer_cell

# Get gates close to final positions so parasitics estimate is close.
detailed_placement

# CTS and detailed placement move instances so update parastic estimates.
estimate_parasitics -placement
set_propagated_clock [all_clocks]
repair_hold_violations -buffer_cell $hold_buffer_cell

detailed_placement
filler_placement $filler_cells
check_placement

# post cts timing report (propagated clocks)
report_checks -path_delay min_max -format full_clock_expanded \
  -fields {input_pin slew capacitance} -digits 3

set cts_def [make_result_file ${design}_${platform}_cts.def]
write_def $cts_def

# missing mysterious vsrc file
#analyze_power_grid

################################################################
# Global routing
set route_guide [make_result_file ${design}_${platform}.route_guide]
foreach layer_adjustment $global_routing_layer_adjustments {
  lassign $layer_adjustment layer adjustment
  set_global_routing_layer_adjustment $layer $adjustment
}
fastroute -guide_file $route_guide \
  -layers $global_routing_layers \
  -clock_layers $global_routing_clock_layers \
  -unidirectional_routing \
  -overflow_iterations 100 \
  -verbose 2

################################################################
# Final Report

# Use global routing based parasitics inlieu of rc extraction
estimate_parasitics -global_routing

report_checks -path_delay min_max -format full_clock_expanded \
  -fields {input_pin slew capacitance} -digits 3
report_wns
report_tns
report_check_types -max_slew -max_capacitance -max_fanout -violators
report_power

report_floating_nets -verbose
report_design_area

set verilog_file [make_result_file ${design}_${platform}.v]
write_verilog -remove_cells $filler_cells $verilog_file

################################################################
# Detailed routing

set detailed_routing 1
set drv_count 0
if { $detailed_routing } {
  set routed_def [make_result_file ${design}_${platform}_route.def]

  set tr_lef [make_tr_lef]
  set tr_params [make_tr_params $tr_lef $cts_def $route_guide $routed_def]
  if { [catch "exec which TritonRoute"] } {
    error "TritonRoute not found."
  }
  # TritonRoute returns error even when successful.
  catch "exec TritonRoute $tr_params" tr_log
  puts $tr_log
  regexp -all {number of violations = ([0-9]+)} $tr_log ignore drv_count
}

################################################################
set pass 1

if { ![info exists drv_count] } {
  puts "Fail: drv count not found."
  set pass 0
} elseif { $drv_count > $max_drv_count } {
  puts "Fail: max drv count exceeded $drv_count > $max_drv_count."
  set pass 0
}

if { [sta::worst_slack -max] < $setup_slack_limit } {
  puts "Fail: setup slack limit exceeded [format %.2f [sta::worst_slack -max]] < $setup_slack_limit"
  set pass 0
}

if { [sta::worst_slack -min] < $hold_slack_limit } {
  puts "Fail: hold slack limit exceeded [format %.2f [sta::worst_slack -min]] < $hold_slack_limit"
  set pass 0
}

if { $pass } {
  puts "pass"
} else {
  puts "fail"
}
