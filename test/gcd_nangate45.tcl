# gcd flow pipe cleaner
source "helpers.tcl"
source "Nangate45/Nangate45.vars"
source "gcd_nangate45.vars"

read_lef $lef_file
read_liberty $liberty_file
read_verilog $synth_verilog
link_design $design
read_sdc $sdc_file

eval $init_floorplan_cmd

io_placer -random -hor_layer $io_placer_hor_layer -ver_layer $io_placer_ver_layer

eval $tapcell_cmd

pdngen -verbose $pdn_cfg

# pre-placement/sizing wireload timing
report_checks

global_placement -disable_routability_driven -density $place_density \
  -pad_left $global_place_pad -pad_right $global_place_pad

# resize
set_wire_rc -layer $wire_rc_layer
set_dont_use $dont_use
resize
buffer_ports -buffer_cell $resize_buffer_cell
repair_max_cap -buffer_cell $resize_buffer_cell
repair_max_slew -buffer_cell $resize_buffer_cell
repair_max_fanout -max_fanout $max_fanout -buffer_cell $resize_buffer_cell
repair_tie_fanout -max_fanout $max_fanout $tielo_port
repair_tie_fanout -max_fanout $max_fanout $tiehi_port
repair_hold_violations -buffer_cell $resize_buffer_cell
resize

clock_tree_synthesis -lut_file $cts_lut_file \
  -sol_list $cts_sol_file \
  -root_buf $cts_buffer \
  -wire_unit 20
# CTS changed the network behind the STA's back.
sta::network_changed

set_placement_padding -global -left $detail_place_pad -right $detail_place_pad
detailed_placement
optimize_mirroring
filler_placement $filler_cells
check_placement

set cts_def [make_result_file ${design}_cts.def]
write_def $cts_def

# missing vsrc file
#analyze_power_grid

set route_guide [make_result_file ${design}.route_guide]
fastroute -output_file $route_guide\
          -max_routing_layer 10 \
          -unidirectional_routing true \
          -capacity_adjustment 0.15 \
          -layers_adjustments {{2 0.5} {3 0.5}} \
          -overflow_iterations 100

set routed_def [make_result_file ${design}_route.def]

# This is why parameter files suck.
set tr_params [make_result_file tr.params]
set tr_param_stream [open $tr_params "w"]
puts $tr_param_stream "lef:$triton_route_lef"
puts $tr_param_stream "def:$cts_def"
puts $tr_param_stream "guide:$route_guide"
puts $tr_param_stream "output:$routed_def"
puts $tr_param_stream "outputTA:[make_result_file "${design}_route_TA.def"]"
puts $tr_param_stream "outputguide:[make_result_file "${design}_output_guide.mod"]"
puts $tr_param_stream "outputDRC:[make_result_file "${design}_route_drc.rpt"]"
puts $tr_param_stream "outputMaze:[make_result_file "${design}_maze.log"]"
puts $tr_param_stream "threads:1"
puts $tr_param_stream "cpxthreads:1"
puts $tr_param_stream "verbose:1"
puts $tr_param_stream "gap:0"
puts $tr_param_stream "timeout:2400"
close $tr_param_stream

catch "exec TritonRoute $tr_params" tr_log
puts $tr_log
regexp -all {number of violations = ([0-9]+)} $tr_log ignore drv_count

# Need a way to clear db
#read_def $routed_def

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

set max_drv_count 3
if { $drv_count >  $max_drv_count } {
  exit 1
}
