# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -pin_names {req_msg*} -region bottom:0-18
set_io_pin_constraint -pin_names {resp_msg*} -region bottom:10-20

place_pins -hor_layers metal3 -ver_layers metal2

set def_file [make_result_file add_constraint8.def]
write_def $def_file

diff_file add_constraint8.defok $def_file
