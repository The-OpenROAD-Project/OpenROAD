# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -pin_names {resp_val resp_rdy req_rdy req_val req_msg*} -region bottom:*
set_io_pin_constraint -pin_names {req_msg[15] req_msg[14] resp_msg[15] resp_msg[14]} -region top:*

place_pins -hor_layers metal3 -ver_layers metal2 -random

set def_file1 [make_result_file add_constraint6_1.def]
write_def $def_file1

clear_io_pin_constraints
place_pins -hor_layers metal3 -ver_layers metal2

set def_file2 [make_result_file add_constraint6_2.def]
write_def $def_file2

diff_file add_constraint6_1.defok $def_file1
diff_file add_constraint6_2.defok $def_file2
