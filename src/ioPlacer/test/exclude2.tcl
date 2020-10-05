# exclude vertical edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

io_placer -hor_layer 2 -ver_layer 3 -boundaries_offset 0 -min_distance 1 -exclude left:* -exclude right:*

set def_file [make_result_file exclude2.def]

write_def $def_file

diff_file exclude2.defok $def_file
