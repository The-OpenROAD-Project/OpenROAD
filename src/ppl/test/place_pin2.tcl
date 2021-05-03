# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pin -pin_name clk -layer metal7 -location {40 30} -pin_size {1.6 2.5}
place_pin -pin_name resp_val -layer metal4 -location {12 50} -pin_size {2 2}
place_pin -pin_name req_msg[0] -layer metal10 -location {25 70} -pin_size {4 4}

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file [make_result_file place_pin2.def]

write_def $def_file

diff_file place_pin2.defok $def_file