
set clock_cycle 900 
set uncertainty 0
set io_delay 0


set clock_port clk

create_clock -name clk -period $clock_cycle [get_ports $clock_port]
set_clock_uncertainty $uncertainty [all_clocks]

set_input_delay -clock [get_clocks clk] -add_delay -max $io_delay [all_inputs]
set_output_delay -clock [get_clocks clk] -add_delay -max $io_delay [all_outputs]
