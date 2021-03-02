# 2 corners with wire parasitics
define_corners ss ff
read_liberty -corner ss Nangate45/Nangate45_slow.lib
read_liberty -corner ff Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_def reg6.def

set_timing_derate -early 0.9
set_timing_derate -late  1.1
create_clock -period 10 clk
set_input_delay -clock clk 0 in1

set_wire_rc -layer metal1
estimate_parasitics -placement

# report all corners
report_checks -path_delay min_max
report_checks -corner ss -path_delay max
report_checks -corner ff -path_delay min
