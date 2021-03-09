# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def no_instance_pins.def

place_pins -hor_layers 2 -ver_layers 3 -corner_avoidance 0 -min_distance 1 -exclude top:* -exclude bottom:*

set def_file [make_result_file no_instance_pins.def]

write_def $def_file

diff_file no_instance_pins.defok $def_file