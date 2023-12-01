# exclude horizontal edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -pin_names {resp_val resp_rdy req_rdy} -group
set_io_pin_constraint -pin_names {req_msg[15] req_msg[14] resp_msg[15] resp_msg[14]} -group

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file [make_result_file group_pins8.def]

write_def $def_file

diff_file group_pins8.defok $def_file
