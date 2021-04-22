# exclude regions in two edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12 -exclude right:15-95 -exclude bottom:10-70

set def_file [make_result_file exclude3.def]

write_def $def_file

diff_file exclude3.defok $def_file
