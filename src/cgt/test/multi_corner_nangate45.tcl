# clock_gating with more than one corner defined
include "helpers.tcl"
define_corners fast slow
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_verilog enable_reg_nangate45.v
link_design enable_reg
create_clock [get_ports clk] -name clock -period 0.5
check_error "clock_gating without -corner" {
  clock_gating -min_instances 1
} STA-0103
clock_gating -min_instances 1 -corner slow
set verilog_file [make_result_file multi_corner_nangate45_gated.v]
write_verilog $verilog_file
report_file $verilog_file
