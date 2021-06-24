############################################################################
##
## Copyright (c) 2019, The Regents of the University of California
## All rights reserved.
##
## BSD 3-Clause License
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
############################################################################

# Assumes flow_helpers.tcl has been read.
read_libraries
read_verilog $synth_verilog
link_design $top_module
read_sdc $sdc_file

utl::metric "ord_sha1" [ord::openroad_git_sha1]
# Note that sta::network_instance_count is not valid after tapcells are added.
utl::metric "instance_count" [sta::network_instance_count]

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
source $layer_rc_file
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
set global_place_def [make_result_file ${design}_${platform}_global_place.def]
write_def $global_place_def

################################################################
# Repair max slew/cap/fanout violations and normalize slews

estimate_parasitics -placement

repair_design

repair_tie_fanout -separation $tie_separation $tielo_port
repair_tie_fanout -separation $tie_separation $tiehi_port

set_placement_padding -global -left $detail_place_pad -right $detail_place_pad
detailed_placement

# post resize timing report (ideal clocks)
report_worst_slack -min -digits 3
report_worst_slack -max -digits 3
report_tns -digits 3
# Check slew repair
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
set cts_def [make_result_file ${design}_${platform}_cts.def]
write_def $cts_def

################################################################
# Setup/hold timing repair

estimate_parasitics -placement
set_propagated_clock [all_clocks]
repair_timing

# Post timing repair using placement based parasitics.
report_worst_slack -min -digits 3
report_worst_slack -max -digits 3
report_tns -digits 3

detailed_placement
# Capture utilization before fillers make it 100%
utl::metric "utilization" [format %.1f [expr [rsz::utilization] * 100]]
utl::metric "design_area" [sta::format_area [rsz::design_area] 0]
filler_placement $filler_cells
set dpl_errors [check_placement -verbose]
utl::metric "DPL::errors" $dpl_errors

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

set antenna_report [make_result_file ${design}_${platform}_ant.log]
set antenna_errors [check_antennas -report_violating_nets -report_file $antenna_report]

utl::metric "ANT::errors" $antenna_errors

if { $antenna_errors > 0 } {
  fail "found $antenna_errors antenna violations"
}

set verilog_file [make_result_file ${design}_${platform}.v]
write_verilog -remove_cells $filler_cells $verilog_file

################################################################
# Detailed routing

set_thread_count [exec getconf _NPROCESSORS_ONLN]
detailed_route -guide $route_guide \
               -output_guide [make_result_file "${design}_${platform}_output_guide.mod"] \
               -output_drc [make_result_file "${design}_${platform}_route_drc.rpt"] \
               -output_maze [make_result_file "${design}_${platform}_maze.log"] \
               -verbose 0

set drv_count [detailed_route_num_drvs]
utl::metric "DRT::drv" $drv_count

set routed_def [make_result_file ${design}_${platform}_route.def]
write_def $routed_def

################################################################
# Extraction

if { $rcx_rules_file != "" } {
  define_process_corner -ext_model_index 0 X
  extract_parasitics -ext_model_file $rcx_rules_file

  set spef_file [make_result_file ${design}_${platform}.spef]
  write_spef $spef_file

  read_spef $spef_file
} else {
  # Use global routing based parasitics inlieu of rc extraction
  estimate_parasitics -global_routing
}

################################################################
# Final Report

report_checks -path_delay min_max -format full_clock_expanded \
  -fields {input_pin slew capacitance} -digits 3
report_worst_slack -min -digits 3
report_worst_slack -max -digits 3
report_tns -digits 3
report_check_types -max_slew -max_capacitance -max_fanout -violators -digits 3
report_clock_skew -digits 3
report_power -corner $power_corner

report_floating_nets -verbose
report_design_area

utl::metric "worst_slack_min" [sta::worst_slack -min]
utl::metric "worst_slack_max" [sta::worst_slack -max]
utl::metric "tns_max" [sta::total_negative_slack -max]
utl::metric "clock_skew" [sta::worst_clock_skew -setup]
utl::metric "max_slew_violations" [sta::max_slew_violation_count]
utl::metric "max_fanout_violations" [sta::max_fanout_violation_count]
utl::metric "max_capacitance_violations" [sta::max_capacitance_violation_count]
utl::metric "clock_period" [get_property [lindex [all_clocks] 0] period]

# not really useful without pad locations
#set_pdnsim_net_voltage -net $vdd_net_name -voltage $vdd_voltage
#analyze_power_grid -net $vdd_net_name
