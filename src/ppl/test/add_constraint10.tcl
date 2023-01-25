# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -pin_names {resp_msg[0] resp_msg[1] clk resp_val resp_rdy resp_msg[10] resp_msg[11] resp_msg[12] resp_msg[13]} -region bottom:*
set_io_pin_constraint -pin_names {resp_msg[3] resp_msg[2] resp_msg[14] req_val req_rdy req_msg[10] req_msg[11] req_msg[12] req_msg[13]} -region top:0-18
set_io_pin_constraint -mirrored_pins {resp_msg[0] req_msg[0] resp_msg[1] req_msg[1] resp_msg[2] req_msg[2] resp_msg[3] req_msg[3]}
place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file [make_result_file add_constraint10.def]

write_def $def_file

diff_file add_constraint10.defok $def_file