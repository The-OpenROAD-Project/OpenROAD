source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_clk_inverters7.v
link_design top
create_clock -period 1 clk

repair_clock_inverters

sta::check_axioms
report_net clk
set verilog_file [make_result_file repair_clk_inverters7_out.v]
write_verilog $verilog_file
report_file $verilog_file
