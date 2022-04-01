create_clock -period 10 -name clk1 [get_ports clk1]
create_clock -period 20 -name clk2 [get_ports clk2]
set_input_delay 5 -clock [get_clocks clk1] [get_ports in]
set_input_delay 10 -clock [get_clocks clk2] [get_ports in]
set_output_delay 5 -clock [get_clocks clk1] [get_ports out]
set_output_delay 10 -clock [get_clocks clk2] [get_ports out]

