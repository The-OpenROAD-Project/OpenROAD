# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -direction INPUT -region top:*
io_placer -hor_layers 2 -ver_layers 3 -boundaries_offset 0 -min_distance 1

set def_file [make_result_file add_constraint1.def]

write_def $def_file

diff_file add_constraint1.defok $def_file