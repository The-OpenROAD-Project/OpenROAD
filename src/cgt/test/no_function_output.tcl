# clock_gating with a library cell whose output pin has no function
include "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty no_function_output.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef no_function_output.lef
read_verilog enable_reg_nangate45.v
link_design enable_reg
create_clock [get_ports clk] -name clock -period 0.5
clock_gating -min_instances 1
set verilog_file [make_result_file no_function_output_gated.v]
write_verilog $verilog_file
report_file $verilog_file
