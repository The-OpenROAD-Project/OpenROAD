source "flow_helpers.tcl"

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

io_placer -random -hor_layer $io_placer_hor_layer -ver_layer $io_placer_ver_layer

if { [have_macros] } {
  # tdms_place (but replace isn't timing driven)
  global_placement -disable_routability_driven -density $global_place_density

  macro_placement -global_config $ip_global_cfg
}

eval $tapcell_cmd

pdngen -verbose $pdn_cfg

# pre-placement/sizing wireload timing
report_checks

global_placement -disable_routability_driven \
  -density $global_place_density \
  -init_density_penalty $global_place_density_penalty \
  -pad_left $global_place_pad -pad_right $global_place_pad

# resize
set_wire_rc -layer $wire_rc_layer
estimate_parasitics -placement
set_dont_use $dont_use

repair_design -max_wire_length $max_wire_length \
  -buffer_cell $resize_buffer_cell
resize

repair_tie_fanout -separation $tie_separation $tielo_port
repair_tie_fanout -separation $tie_separation $tiehi_port

set_placement_padding -global -left $detail_place_pad -right $detail_place_pad
detailed_placement
optimize_mirroring
check_placement -verbose

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

# CTS trashes the timing state and detailed placement moves instances
# so update parastic estimates.
estimate_parasitics -placement
set_propagated_clock [all_clocks]
repair_hold_violations -buffer_cell $hold_buffer_cell

detailed_placement
filler_placement $filler_cells
check_placement

set cts_def [make_result_file ${design}_${platform}_cts.def]
write_def $cts_def

# missing vsrc file
#analyze_power_grid

set route_guide [make_result_file ${design}_${platform}.route_guide]
fastroute -output_file $route_guide\
  -max_routing_layer $max_routing_layer \
  -unidirectional_routing true \
  -layers_adjustments $layers_adjustments \
  -layers_pitches $layers_pitches \
  -overflow_iterations 100 \
  -verbose 2

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

################################################################

# Reinitialize libraries and db with routed def.
ord::clear
read_libraries
read_def $routed_def
read_sdc $sdc_file

# final report
# inlieu of rc extraction
set_wire_rc -layer $wire_rc_layer
report_checks -path_delay min_max
report_wns
report_tns
report_check_types -max_slew -violators
report_power

report_floating_nets -verbose
report_design_area

set verilog_file [make_result_file ${design}_${platform}.v]
write_verilog -remove_cells $filler_cells $verilog_file

if { ![info exists drv_count] } {
  puts "fail drv count not found."
} elseif { $drv_count > $max_drv_count } {
  puts "fail max drv count exceeded $drv_count > $max_drv_count."
} else {
  puts "pass drv count $drv_count <= $max_drv_count."
}
