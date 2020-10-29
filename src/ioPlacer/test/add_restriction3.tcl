# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_pin_restriction -direction INPUT -region left:*
io_placer -hor_layer 2 -ver_layer 3 -boundaries_offset 0 -min_distance 1

set def_file [make_result_file add_restriction3.def]

write_def $def_file

diff_file add_restriction3.defok $def_file