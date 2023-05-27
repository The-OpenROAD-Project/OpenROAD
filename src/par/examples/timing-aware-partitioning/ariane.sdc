# ####################################################################

#  Created by Genus(TM) Synthesis Solution 21.10-p002_1 on Mon Sep 19 01:18:11 PDT 2022

# ####################################################################

set sdc_version 2.0

# Set the current design
current_design ariane

create_clock -name "core_clock" -period 1.0 -waveform {0.0 0.5} [get_ports clk_i]
set_clock_gating_check -setup 0.0 
