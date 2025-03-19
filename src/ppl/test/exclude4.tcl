# exclude regions in two edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

exclude_io_pin_region -region right:15-95 -region bottom:10-70
place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file [make_result_file exclude4.def]

write_def $def_file

diff_file exclude4.defok $def_file
