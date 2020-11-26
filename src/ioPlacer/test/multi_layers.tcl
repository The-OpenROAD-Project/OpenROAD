# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

io_placer -hor_layer {2 4} -ver_layer {3 5} -boundaries_offset 0 -min_distance 1

set def_file [make_result_file multi_layer.def]

write_def $def_file

diff_file multi_layers.defok $def_file
