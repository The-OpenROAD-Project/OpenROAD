# exclude horizontal edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers 2 -ver_layers 3 -corner_avoidance 0 -min_distance 1 -group_pins {resp_val resp_rdy req_rdy} -group_pins {req_msg[15] req_msg[14] resp_msg[15] resp_msg[14]}

set def_file [make_result_file group_pins1.def]

write_def $def_file

diff_file group_pins1.defok $def_file
