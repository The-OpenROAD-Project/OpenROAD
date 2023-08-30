# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -region bottom:* -group -order -pin_names {req_msg[0] req_msg[1] req_msg[2] req_msg[3] req_msg[4] req_msg[5] req_msg[6] req_msg[7] req_msg[8] req_msg[9] req_msg[10]}
set_io_pin_constraint -mirrored_pins {resp_msg[0] req_msg[0] resp_msg[1] req_msg[1] resp_msg[2] req_msg[2] resp_msg[3] req_msg[3] resp_msg[4] req_msg[4] resp_msg[5] req_msg[5] resp_msg[6] req_msg[6] resp_msg[7] req_msg[7] resp_msg[8] req_msg[8] resp_msg[9] req_msg[9] resp_msg[10] req_msg[10]}

set tcl_file [make_result_file write_pin_placement3.tcl]
set def_file [make_result_file write_pin_placement3.def]

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12 -annealing -write_pin_placement $tcl_file

source $tcl_file

write_def $def_file

diff_file write_pin_placement3.defok $def_file

diff_file write_pin_placement3.tclok $tcl_file
