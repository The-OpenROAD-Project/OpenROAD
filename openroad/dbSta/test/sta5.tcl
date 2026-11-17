# 3 corners with +/- 10% derating example
source "helpers.tcl"
read_liberty example1_slow.lib
read_liberty example1_typ.lib
read_liberty example1_fast.lib
define_scene ss -liberty example1_slow.lib
define_scene tt -liberty example1_typ.lib
define_scene ff -liberty example1_fast.lib
set_scene ss
read_lef example1.lef
read_def example1.def

set_timing_derate -early 0.9
set_timing_derate -late 1.1
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
# report all corners
report_checks -path_delay min_max
# report typical corner
report_checks -scenes tt
