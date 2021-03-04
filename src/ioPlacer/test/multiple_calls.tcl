# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers 2 -ver_layers 3 -corner_avoidance 0 -min_distance 1

set def_file1 [make_result_file mc1_iop.def]

write_def $def_file1

ppl::clear_ioplacer

place_pins -hor_layers 2 -ver_layers 3 -corner_avoidance 0 -min_distance 1

set def_file2 [make_result_file mc2_iop.def]

write_def $def_file2

diff_file $def_file1 $def_file2