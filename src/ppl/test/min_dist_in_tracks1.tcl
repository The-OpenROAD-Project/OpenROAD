# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 1 -min_distance_in_tracks

set def_file [make_result_file min_dist_in_tracks1.def]

write_def $def_file

diff_file min_dist_in_tracks1.defok $def_file