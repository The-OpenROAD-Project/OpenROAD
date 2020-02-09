# read_verilog, sdf
read_liberty example1_slow.lib
read_lef example1.lef
read_verilog example1.v
link_design top

read_sdf example1.sdf
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
report_checks
