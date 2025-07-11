create_clock [get_ports clk] -name core_clock -period 0.5
set_input_delay 0.1 [all_inputs] -clock core_clock
set_output_delay 0.1 [all_outputs] -clock core_clock
