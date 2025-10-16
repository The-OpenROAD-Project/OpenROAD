###############################################################################
# Created by write_sdc
# Tue Feb 22 14:35:57 2022
###############################################################################
current_design gcd
###############################################################################
# Timing Constraints
###############################################################################
create_clock -name core_clock -period 4.3647 [get_ports {clk}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[0]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[10]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[11]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[12]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[13]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[14]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[15]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[16]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[17]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[18]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[19]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[1]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[20]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[21]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[22]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[23]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[24]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[25]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[26]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[27]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[28]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[29]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[2]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[30]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[31]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[3]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[4]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[5]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[6]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[7]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[8]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_msg[9]}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_val}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {reset}]
set_input_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_rdy}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {req_rdy}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[0]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[10]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[11]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[12]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[13]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[14]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[15]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[1]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[2]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[3]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[4]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[5]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[6]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[7]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[8]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_msg[9]}]
set_output_delay 0.8729 -clock [get_clocks {core_clock}] -add_delay [get_ports {resp_val}]
###############################################################################
# Environment
###############################################################################
###############################################################################
# Design Rules
###############################################################################
