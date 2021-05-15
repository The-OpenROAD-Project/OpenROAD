# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -pin_names {resp_val resp_rdy req_rdy req_val req_msg* *msg*} -region bottom:*

place_pins -hor_layers metal3 -ver_layers metal2 -random

set def_file [make_result_file add_constraint7.def]
write_def $def_file

diff_file add_constraint7.defok $def_file
