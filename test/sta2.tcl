# read_verilog, sdf
read_liberty liberty1.lib
read_lef liberty1.lef
read_verilog reg1.v
link_design top

read_sdf reg1.sdf
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
report_checks
