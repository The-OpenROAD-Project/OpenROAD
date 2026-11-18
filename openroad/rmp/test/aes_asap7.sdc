create_clock [get_ports clk] -name core_clock -period 1000

set_input_delay 200 -clock core_clock [all_inputs]
set_output_delay 200 -clock core_clock [all_outputs]
