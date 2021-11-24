current_design aes_rcon

set_case_analysis 0 [get_ports {kld}]
create_clock -name clk -period 0.8109 -waveform {0.0000 0.4054} [get_ports {clk}]

