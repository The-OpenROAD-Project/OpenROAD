# min/max delay calc
source "helpers.tcl"
read_lef example1.lef
read_liberty example1_fast.lib
read_liberty example1_slow.lib
define_scene default -liberty_min example1_fast.lib -liberty_max example1_slow.lib
read_def example1.def

create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
set_output_delay -clock clk 0 out
report_checks -path_delay min_max
