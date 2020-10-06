# exclude horizontal edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

io_placer -hor_layer 2 -ver_layer 3 -boundaries_offset 0 -min_distance 1 -exclude top:* -exclude bottom:*

set def_file [make_result_file exclude1.def]

write_def $def_file

diff_file exclude1.defok $def_file
