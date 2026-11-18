# clock_gating with more than one corner defined
include "helpers.tcl"
read_liberty Nangate45/Nangate45_fast.lib
read_liberty Nangate45/Nangate45_slow.lib
define_scene fast -liberty Nangate45/Nangate45_fast.lib
define_scene slow -liberty Nangate45/Nangate45_slow.lib
set_scene fast
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_verilog enable_reg_nangate45.v
link_design enable_reg
create_clock [get_ports clk] -name clock -period 0.5
clock_gating -min_instances 1
clock_gating -min_instances 1 -scene slow
set verilog_file [make_result_file multi_corner_nangate45_gated.v]
write_verilog $verilog_file
report_file $verilog_file
