source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_clk_inverters10_floating.v
link_design top
create_clock -period 1 [get_pins c1/ZN]

repair_clock_inverters

report_net clk_n
set verilog_file [make_result_file repair_clk_inverters10_floating_out.v]
write_verilog $verilog_file
report_file $verilog_file
