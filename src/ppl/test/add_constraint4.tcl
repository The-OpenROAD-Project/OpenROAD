# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -direction INPUT -region right:*
place_pins -hor_layers 2 -ver_layers 3 -corner_avoidance 0 -min_distance 1

set def_file [make_result_file add_constraint4.def]

write_def $def_file

diff_file add_constraint4.defok $def_file