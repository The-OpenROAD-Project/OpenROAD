###############################################################################
# Created by write_sdc
# Mon Jul 17 00:38:21 2023
###############################################################################
current_design asic_top
###############################################################################
# Timing Constraints
###############################################################################
create_clock -name clk -period 45.0000 [get_pins {padring.we_pads[0].i0.padio[5].i0.gpio/IN}]
set_clock_uncertainty 0.0000 clk
###############################################################################
# Environment
###############################################################################
set_load -pin_load 0.0231 [get_ports {vdd}]
set_load -pin_load 0.0231 [get_ports {vddio}]
set_load -pin_load 0.0231 [get_ports {vss}]
set_load -pin_load 0.0231 [get_ports {vssio}]
set_load -pin_load 0.0231 [get_ports {ea_pad[8]}]
set_load -pin_load 0.0231 [get_ports {ea_pad[7]}]
set_load -pin_load 0.0231 [get_ports {ea_pad[6]}]
set_load -pin_load 0.0231 [get_ports {ea_pad[5]}]
set_load -pin_load 0.0231 [get_ports {ea_pad[4]}]
set_load -pin_load 0.0231 [get_ports {ea_pad[3]}]
set_load -pin_load 0.0231 [get_ports {ea_pad[2]}]
set_load -pin_load 0.0231 [get_ports {ea_pad[1]}]
set_load -pin_load 0.0231 [get_ports {ea_pad[0]}]
set_load -pin_load 0.0231 [get_ports {no_pad[8]}]
set_load -pin_load 0.0231 [get_ports {no_pad[7]}]
set_load -pin_load 0.0231 [get_ports {no_pad[6]}]
set_load -pin_load 0.0231 [get_ports {no_pad[5]}]
set_load -pin_load 0.0231 [get_ports {no_pad[4]}]
set_load -pin_load 0.0231 [get_ports {no_pad[3]}]
set_load -pin_load 0.0231 [get_ports {no_pad[2]}]
set_load -pin_load 0.0231 [get_ports {no_pad[1]}]
set_load -pin_load 0.0231 [get_ports {no_pad[0]}]
set_load -pin_load 0.0231 [get_ports {so_pad[8]}]
set_load -pin_load 0.0231 [get_ports {so_pad[7]}]
set_load -pin_load 0.0231 [get_ports {so_pad[6]}]
set_load -pin_load 0.0231 [get_ports {so_pad[5]}]
set_load -pin_load 0.0231 [get_ports {so_pad[4]}]
set_load -pin_load 0.0231 [get_ports {so_pad[3]}]
set_load -pin_load 0.0231 [get_ports {so_pad[2]}]
set_load -pin_load 0.0231 [get_ports {so_pad[1]}]
set_load -pin_load 0.0231 [get_ports {so_pad[0]}]
set_load -pin_load 0.0231 [get_ports {we_pad[8]}]
set_load -pin_load 0.0231 [get_ports {we_pad[7]}]
set_load -pin_load 0.0231 [get_ports {we_pad[6]}]
set_load -pin_load 0.0231 [get_ports {we_pad[5]}]
set_load -pin_load 0.0231 [get_ports {we_pad[4]}]
set_load -pin_load 0.0231 [get_ports {we_pad[3]}]
set_load -pin_load 0.0231 [get_ports {we_pad[2]}]
set_load -pin_load 0.0231 [get_ports {we_pad[1]}]
set_load -pin_load 0.0231 [get_ports {we_pad[0]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {vdd}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {vddio}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {vss}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {vssio}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {ea_pad[8]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {ea_pad[7]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {ea_pad[6]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {ea_pad[5]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {ea_pad[4]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {ea_pad[3]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {ea_pad[2]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {ea_pad[1]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {ea_pad[0]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {no_pad[8]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {no_pad[7]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {no_pad[6]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {no_pad[5]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {no_pad[4]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {no_pad[3]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {no_pad[2]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {no_pad[1]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {no_pad[0]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {so_pad[8]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {so_pad[7]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {so_pad[6]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {so_pad[5]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {so_pad[4]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {so_pad[3]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {so_pad[2]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {so_pad[1]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {so_pad[0]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {we_pad[8]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {we_pad[7]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {we_pad[6]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {we_pad[5]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {we_pad[4]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {we_pad[3]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {we_pad[2]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {we_pad[1]}]
set_driving_cell -lib_cell sky130_fd_sc_hd__buf_1 -pin {X} \
  -input_transition_rise 0.0000 -input_transition_fall 0.0000 [get_ports {we_pad[0]}]
###############################################################################
# Design Rules
###############################################################################
