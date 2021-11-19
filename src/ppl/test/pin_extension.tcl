# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_pin_length -hor_length 1.0 -ver_length 1.0

set_pin_length_extension -hor_extension 0.3 -ver_extension 0.2

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file [make_result_file pin_extension.def]

write_def $def_file

diff_file pin_extension.defok $def_file