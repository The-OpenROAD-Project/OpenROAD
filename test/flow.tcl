# assumes flow_helpers.tcl has been read
read_libraries
read_verilog $synth_verilog
link_design $top_module
read_sdc $sdc_file

initialize_floorplan -site $site \
  -die_area $die_area \
  -core_area $core_area

source $tracks_file

# remove buffers inserted by synthesis 
remove_buffers

################################################################
# IO Placement (random)
place_pins -random -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer

################################################################
# Macro Placement
if { [have_macros] } {
  global_placement -density $global_place_density
  macro_placement -halo $macro_place_halo -channel $macro_place_channel
}

################################################################
# Tapcell insertion
eval tapcell $tapcell_args

################################################################
# Power distribution network insertion
pdngen -verbose $pdn_cfg

################################################################
# Global placement

# Used by resizer for timing driven placement.
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock  -layer $wire_rc_layer_clk
set_dont_use $dont_use

# If/when this enables routing driven also the layer adjustments have to
# move to here.
global_placement -timing_driven -density $global_place_density \
  -init_density_penalty $global_place_density_penalty \
  -pad_left $global_place_pad -pad_right $global_place_pad

# IO Placement
place_pins -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer

# checkpoint
set global_place_db [make_result_file ${design}_${platform}_global_place.db]
write_db $global_place_db

################################################################
# Repair max slew/cap/fanout violations and normalize slews

estimate_parasitics -placement

repair_design

repair_tie_fanout -separation $tie_separation $tielo_port
repair_tie_fanout -separation $tie_separation $tiehi_port

set_placement_padding -global -left $detail_place_pad -right $detail_place_pad
detailed_placement

# post resize timing report (ideal clocks)
report_worst_slack -min
report_worst_slack -max
report_tns
report_check_types -max_slew -max_capacitance -max_fanout -violators

################################################################
# Clock Tree Synthesis

# Clone clock tree inverters next to register loads
# so cts does not try to buffer the inverted clocks.
repair_clock_inverters

clock_tree_synthesis -root_buf $cts_buffer -buf_list $cts_buffer -sink_clustering_enable

# CTS leaves a long wire from the pad to the clock tree root.
repair_clock_nets

# checkpoint
set cts_db [make_result_file ${design}_${platform}_cts.db]
write_db $cts_db

################################################################
# Setup/hold timing repair

estimate_parasitics -placement
set_propagated_clock [all_clocks]
repair_timing

# Post timing repair using placement based parasitics.
report_worst_slack -min
report_worst_slack -max
report_tns

detailed_placement
filler_placement $filler_cells
check_placement -verbose

################################################################
# Global routing
set route_guide [make_result_file ${design}_${platform}.route_guide]
foreach layer_adjustment $global_routing_layer_adjustments {
  lassign $layer_adjustment layer adjustment
  set_global_routing_layer_adjustment $layer $adjustment
}
set_routing_layers -signal $global_routing_layers \
  -clock $global_routing_clock_layers
global_route -guide_file $route_guide \
  -overflow_iterations 100

################################################################
# Final Report

# Use global routing based parasitics inlieu of rc extraction
estimate_parasitics -global_routing

report_checks -path_delay min_max -format full_clock_expanded \
  -fields {input_pin slew capacitance} -digits 3
report_worst_slack -min
report_worst_slack -max
report_tns
report_check_types -max_slew -max_capacitance -max_fanout -violators
report_clock_skew
report_power -corner $power_corner

report_floating_nets -verbose
report_design_area

if { [sta::worst_slack -max] < $setup_slack_limit } {
  fail "setup slack limit exceeded [format %.2f [sta::worst_slack -max]] < $setup_slack_limit"
}

if { [sta::worst_slack -min] < $hold_slack_limit } {
  fail "hold slack limit exceeded [format %.2f [sta::worst_slack -min]] < $hold_slack_limit"
}

# not really useful without pad locations
#set_pdnsim_net_voltage -net $vdd_net_name -voltage $vdd_voltage
#analyze_power_grid -net $vdd_net_name

set verilog_file [make_result_file ${design}_${platform}.v]
write_verilog -remove_cells $filler_cells $verilog_file

################################################################
# Detailed routing

set tr_params [make_tr_params $route_guide 0]

detailed_route -param $tr_params
set routed_def [make_result_file ${design}_${platform}_route.def]
write_def $routed_def

set drv_count [detailed_route_num_drvs]

if { ![info exists drv_count] } {
  fail "drv count not found."
} elseif { $drv_count > $max_drv_count } {
  fail "max drv count exceeded $drv_count > $max_drv_count."
}

puts "pass"
