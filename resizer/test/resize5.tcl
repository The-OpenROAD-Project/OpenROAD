# resize lef macro bus pins
source "helpers.tcl"
read_liberty liberty1.lib
read_liberty bus1.lib
read_lef bus1.lef
read_def bus1.def
create_clock -name clk -period 10
set_input_delay -clock clk 0 [get_ports in[*]]
set_output_delay -clock clk 0 [get_ports out[*]]
set_load .5 [get_ports out[*]]
resize -buffer_cell snl_bufx2
