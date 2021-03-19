# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def cells_not_placed.def

set_io_pin_constraint -direction INPUT -region top:*
set_io_pin_constraint -direction OUTPUT -region bottom:*

place_pins -hor_layers 3 -ver_layers 2

set def_file [make_result_file cells_not_placed.def]

write_def $def_file

diff_file cells_not_placed.defok $def_file
