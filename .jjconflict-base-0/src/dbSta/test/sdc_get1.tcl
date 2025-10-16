# sdc get_*
source "helpers.tcl"
read_lef example1.lef
read_def example1.def
read_liberty example1_slow.lib
report_object_names [get_ports -of_objects [get_nets clk1]]
report_object_full_names [get_pins -of_objects [get_nets clk1]]
