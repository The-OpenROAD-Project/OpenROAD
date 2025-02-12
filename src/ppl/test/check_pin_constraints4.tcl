# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -pin_names { req_msg[0] req_msg[1] req_msg[2] req_msg[3] req_msg[4] req_msg[5] req_msg[6] } -region bottom:4-18
set_io_pin_constraint -mirrored_pins {resp_msg[0] req_msg[0] resp_msg[1] req_msg[1] resp_msg[2] req_msg[2] resp_msg[3] req_msg[3] resp_msg[4] req_msg[4] resp_msg[5] req_msg[5] resp_msg[6] req_msg[6]}

set_io_pin_constraint -mirrored_pins {resp_msg[7] req_msg[7] resp_msg[8] req_msg[8] resp_msg[9] req_msg[9] resp_msg[10] req_msg[10] resp_msg[11] req_msg[11]}
set_io_pin_constraint -pin_names { req_msg[7] req_msg[8] req_msg[9] req_msg[10] } -region left:40-70

set_io_pin_constraint -pin_names { req_msg[12] req_msg[13] req_msg[14] req_msg[15] req_msg[24] req_msg[25] req_msg[26] } -region right:80-100
set_io_pin_constraint -mirrored_pins {req_msg[12] resp_msg[12] req_msg[13] resp_msg[13] req_msg[14] resp_msg[14] req_msg[15] resp_msg[15]}

set rpt_file [make_result_file check_pin_constraints4.rpt]

ppl::write_pin_constraints $rpt_file

diff_file check_pin_constraints4.rptok $rpt_file
