create_clock [get_ports clk] -name core_clock -period 2
set_max_fanout 100 [current_design]
set_input_delay 0.2 [all_inputs] -clock core_clock
set_output_delay 0.2 [all_outputs] -clock core_clock
