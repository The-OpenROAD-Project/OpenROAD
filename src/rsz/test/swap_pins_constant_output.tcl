# report_swappable_pins on cells with a constant output
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty swap_pins_constant_output.lib
read_verilog swap_pins_constant_output.v
link_design top

rsz::report_swappable_pins
