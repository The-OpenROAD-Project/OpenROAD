# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def no_instance_pins.def

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12 -exclude top:* -exclude bottom:*

set def_file [make_result_file no_instance_pins.def]

write_def $def_file

diff_file no_instance_pins.defok $def_file