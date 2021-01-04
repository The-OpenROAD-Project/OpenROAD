create_clock [get_ports clk] -period 0.1
set_clock_uncertainty 0  [get_clocks clk]
set_input_delay -clock clk  0.02  [get_ports a1]
set_input_delay -clock clk  0.02  [get_ports a2]
set_input_delay -clock clk  0.02  [get_ports a3]
set_output_delay -clock clk  0.02  [get_ports y1]
set_output_delay -clock clk  0.02  [get_ports y2]
