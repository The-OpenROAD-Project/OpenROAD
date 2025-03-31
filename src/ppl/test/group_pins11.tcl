# exclude horizontal edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12 -group_pins {resp_val resp_rdy req_val req_rdy} -group_pins {req_msg*} -group_pins {resp_msg*}

set def_file [make_result_file group_pins11.def]

write_def $def_file

diff_file group_pins11.defok $def_file
