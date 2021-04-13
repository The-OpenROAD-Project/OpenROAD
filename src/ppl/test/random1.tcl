# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

place_pins -hor_layers metal3 -ver_layers metal2 -random

set def_file [make_result_file random1.def]

write_def $def_file

diff_file random1.defok $def_file
