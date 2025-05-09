create_clock [get_ports clk] -name core_clock -period 500

set_input_delay 100 -clock core_clock [all_inputs]
set_output_delay 100 -clock core_clock [all_outputs]