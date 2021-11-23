# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_pin_length -hor_length 1.0 -ver_length 1.0

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file [make_result_file pin_length.def]

write_def $def_file

diff_file pin_length.defok $def_file