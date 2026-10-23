# Two constructs the native form does not carry, both detected from the
# Sdc itself rather than from write_sdc text: a generated clock and a
# port load. The whole Sdc rides as text, and nothing is dropped.
create_clock -name clk -period 10 [get_ports clk1]
create_generated_clock -name gclk -source [get_ports clk1] -divide_by 2 [get_pins r1/Q]
set_input_delay 1 -clock clk [get_ports in1]
set_load 0.5 [get_ports out]
