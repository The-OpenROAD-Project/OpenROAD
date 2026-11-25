# A register on a gated clock keeps that clock when the register it came
# from was removed before it is rebuilt.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_gated_clock_order.v
link_design top
create_clock -period 0.1 clk

retime

set v_file [make_result_file retime_gated_clock_order.v]
write_verilog $v_file
diff_files retime_gated_clock_order.vok $v_file
