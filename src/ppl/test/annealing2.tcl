# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers metal3 \
          -ver_layers metal4 \
          -annealing \
          -group_pins {req_msg[0] req_msg[1] req_msg[2] req_msg[3] req_msg[4] req_msg[5] req_msg[6] req_msg[7] req_msg[8] req_msg[9]} \
          -group_pins {resp_msg[0] resp_msg[1] resp_msg[2] resp_msg[3] resp_msg[4] resp_msg[5] resp_msg[6] resp_msg[7] resp_msg[8] resp_msg[9]}

set def_file [make_result_file annealing2.def]

write_def $def_file

diff_file annealing2.defok $def_file
