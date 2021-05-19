# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -direction INPUT -region top:*
set_io_pin_constraint -direction OUTPUT -region bottom:*
set_io_pin_constraint -pin_names {req_msg[14] req_msg[15] req_msg[16] req_msg[17]} -region left:*
place_pins -hor_layers metal3 -ver_layers metal2 -random -group_pins {req_msg[14] req_msg[15] req_msg[16] req_msg[17]}

set def_file [make_result_file random8.def]

write_def $def_file

diff_file random8.defok $def_file
