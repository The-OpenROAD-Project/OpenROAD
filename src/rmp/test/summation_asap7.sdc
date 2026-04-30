###############################################################################
# Created by write_sdc
###############################################################################
current_design summation
###############################################################################
# Timing Constraints
###############################################################################
create_clock -name core_clock -period 1260.0000 
set_input_delay 252.0000 -clock [get_clocks {core_clock}] -add_delay [get_ports {a[0]}]
set_input_delay 252.0000 -clock [get_clocks {core_clock}] -add_delay [get_ports {a[1]}]
set_input_delay 252.0000 -clock [get_clocks {core_clock}] -add_delay [get_ports {b[0]}]
set_input_delay 252.0000 -clock [get_clocks {core_clock}] -add_delay [get_ports {b[1]}]
set_output_delay 252.0000 -clock [get_clocks {core_clock}] -add_delay [get_ports {sum[0]}]
set_output_delay 252.0000 -clock [get_clocks {core_clock}] -add_delay [get_ports {sum[1]}]
set_output_delay 252.0000 -clock [get_clocks {core_clock}] -add_delay [get_ports {sum[2]}]
set_output_delay 252.0000 -clock [get_clocks {core_clock}] -add_delay [get_ports {sum[3]}]
###############################################################################
# Environment
###############################################################################
###############################################################################
# Design Rules
###############################################################################

