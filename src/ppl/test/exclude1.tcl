# exclude horizontal edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12 -exclude top:* -exclude bottom:*

set def_file [make_result_file exclude1.def]

write_def $def_file

diff_file exclude1.defok $def_file
