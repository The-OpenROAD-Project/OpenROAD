# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file1 [make_result_file mc1_iop.def]

write_def $def_file1

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file2 [make_result_file mc2_iop.def]

write_def $def_file2

diff_file $def_file1 $def_file2
