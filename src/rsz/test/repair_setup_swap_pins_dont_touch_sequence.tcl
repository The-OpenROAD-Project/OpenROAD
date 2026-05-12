# Probe SwapPinsMove dont-touch behavior vs rsz2 swapPins helper.
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog swap_pins_dont_touch.v
link_design td1
create_clock [get_ports clk] -period 0.1
set_clock_uncertainty 0 [get_clocks clk]
set_input_delay -clock clk 0.02 [get_ports a1]
set_input_delay -clock clk 0.02 [get_ports a2]
set_input_delay -clock clk 0.02 [get_ports a3]
set_input_delay -clock clk 0.00 [get_ports a4]
set_input_delay -clock clk 0.00 [get_ports a5]
set_input_delay -clock clk 0.00 [get_ports a6]
set_output_delay -clock clk 0.01 [get_ports y1]
set_output_delay -clock clk 0.01 [get_ports y2]
initialize_floorplan -die_area "0 0 40 1200" \
  -core_area "0 0 40 1200" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
source $tracks_file
place_pins -hor_layers $io_placer_hor_layer \
  -ver_layers $io_placer_ver_layer
global_placement -skip_nesterov_place
detailed_placement
source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
estimate_parasitics -placement
set_dont_touch net1
repair_design
repair_timing -setup -sequence "swap" \
  -skip_last_gasp \
  -max_passes 10 \
  -verbose
report_worst_slack -max
report_tns -digits 3
