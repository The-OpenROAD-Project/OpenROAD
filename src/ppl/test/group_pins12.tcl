# exclude horizontal edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -group -region top:* -pin_names req_msg*
set_io_pin_constraint -group -region bottom:* -pin_names resp_msg*
set_io_pin_constraint -group -region left:* -pin_names {resp_val resp_rdy req_val req_rdy}
set_io_pin_constraint -mirrored_pins {reset clk}

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file [make_result_file group_pins12.def]

write_def $def_file

diff_file group_pins12.defok $def_file
