# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -pin_names {req_msg[14] req_msg[15]} -region top:*
set_io_pin_constraint -mirrored_pins {req_msg[14] resp_msg[14] req_msg[15] resp_msg[15]}

place_pins -hor_layers metal3 -ver_layers metal2 -random

set def_file [make_result_file random9.def]

write_def $def_file

diff_file random9.defok $def_file
