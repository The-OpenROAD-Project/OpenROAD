# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers 3 -ver_layers 2 -corner_avoidance 0 -min_distance 1

set def_file [make_result_file gcd_iop.def]

write_def $def_file

diff_file gcd.defok $def_file