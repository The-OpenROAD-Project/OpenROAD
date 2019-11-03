# resize reg1 verilog
read_liberty liberty1.lib
read_verilog reg1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
resize -buffer_cell [get_lib_cell liberty1/snl_bufx2]
