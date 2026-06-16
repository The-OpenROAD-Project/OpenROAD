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
# Load the placed fixture to keep this test independent of floorplan and placement.
read_def -floorplan_initialize repair_setup_swap_pins_dont_touch_sequence.def
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
