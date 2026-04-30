create_clock -name core_clock -period 15.155 [get_ports clk_i]
set_input_delay 200 -clock core_clock [all_inputs]
set_output_delay 200 -clock core_clock [all_outputs]
