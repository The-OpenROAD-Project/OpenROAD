# exclude horizontal edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers 3 -ver_layers 2 -corner_avoidance 0 -min_distance 1 -exclude top:* -exclude bottom:*

set def_file [make_result_file exclude1.def]

write_def $def_file

diff_file exclude1.defok $def_file
