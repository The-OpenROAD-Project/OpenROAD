# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -region bottom:* -pin_names {resp_msg[0] resp_msg[1] clk resp_val resp_rdy resp_msg[10] resp_msg[11] resp_msg[12]}
set_io_pin_constraint -group -order -pin_names {resp_msg[0] resp_msg[1] clk resp_val resp_rdy resp_msg[10] resp_msg[11] resp_msg[12]}
set_io_pin_constraint -mirrored_pins {resp_msg[0] req_msg[0] resp_msg[1] req_msg[1] resp_val req_val resp_rdy req_rdy resp_msg[10] req_msg[10]}
place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file [make_result_file add_constraint11.def]

write_def $def_file

diff_file add_constraint11.defok $def_file