# lef/def reg1
read_liberty liberty1.lib
read_lef liberty1.lef
read_def reg1.def
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
