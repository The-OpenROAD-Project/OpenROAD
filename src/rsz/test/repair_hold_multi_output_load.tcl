# Netlist-based regression for issue #10213: repair_timing -hold must not pass
# internal output pins from multi-output cells to insertBufferBeforeLoads.
source "helpers.tcl"
source asap7/asap7.vars

foreach lib [concat [list $liberty_file] $extra_liberty] {
  read_liberty $lib
}
read_lef $tech_lef
read_lef $std_cell_lef
foreach lef $extra_lef { read_lef $lef }

read_verilog repair_hold_multi_output_load.v
link_design gcd

set clk_period 77.5
create_clock -name core_clock -period $clk_period [get_ports clk]
set non_clock_inputs [all_inputs -no_clocks]
set_input_delay [expr $clk_period * 0.2] -clock core_clock $non_clock_inputs
set_output_delay [expr $clk_period * 0.2] -clock core_clock [all_outputs]
set_clock_uncertainty -hold 62 [all_clocks]

source asap7/setRC.tcl

initialize_floorplan \
  -site $site \
  -die_area {0 0 8.977 8.977} \
  -core_area {0.540 0.540 8.424 8.370}
source asap7/asap7.tracks.tcl
set_dont_use $dont_use

global_placement -skip_io -density 0.35 -pad_left 0 -pad_right 0
place_pins -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer
global_placement -density 0.35 -pad_left 0 -pad_right 0 \
  -routability_driven \
  -timing_driven \
  -force_center_initial_place \
  -min_phi_coef 0.95 \
  -max_phi_coef 1.05
repair_design -verbose
detailed_placement
estimate_parasitics -placement

clock_tree_synthesis -sink_clustering_enable -repair_clock_nets
estimate_parasitics -placement
detailed_placement
estimate_parasitics -placement

repair_timing -hold -allow_setup_violations \
  -hold_margin 124 \
  -setup_margin -1000 \
  -max_buffer_percent 80 \
  -max_passes 3 \
  -max_iterations 3 \
  -verbose
