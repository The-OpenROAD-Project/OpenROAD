create_clock [get_ports clk] -name core_clock -period 2
set_max_fanout 100 [current_design]
