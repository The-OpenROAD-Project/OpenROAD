# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -direction INPUT -region top:*
set_io_pin_constraint -direction OUTPUT -region bottom:*
place_pins -hor_layers metal3 -ver_layers metal2 -random

set def_file [make_result_file random3.def]

write_def $def_file

diff_file random3.defok $def_file
