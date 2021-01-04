###################################################################

# Created by write_sdc on Sat Dec 26 05:16:27 2020

###################################################################
set sdc_version 2.0

set_units -time ns -resistance MOhm -capacitance fF -voltage V -current mA
create_clock [get_ports clk]  -period 0.1  -waveform {0 0.05}
set_clock_uncertainty 0  [get_clocks clk]
set_input_delay -clock clk  0.02  [get_ports a1]
set_input_delay -clock clk  0.02  [get_ports a2]
set_input_delay -clock clk  0.02  [get_ports a3]
set_output_delay -clock clk  0.02  [get_ports y1]
set_output_delay -clock clk  0.02  [get_ports y2]
