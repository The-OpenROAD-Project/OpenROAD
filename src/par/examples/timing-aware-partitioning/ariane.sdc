set sdc_version 2.0

# Set the current design
current_design ariane

create_clock -name "core_clock" -period 1.0 -waveform {0.0 0.5} [get_ports clk_i]
set_clock_gating_check -setup 0.0 
