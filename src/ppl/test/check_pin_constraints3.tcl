# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -pin_names { req_msg[0] req_msg[1] req_msg[2] req_msg[3] req_msg[4] req_msg[5] req_msg[6] req_msg[7] } -region bottom:4-18
set_io_pin_constraint -mirrored_pins {resp_msg[0] req_msg[0] resp_msg[1] req_msg[1] resp_msg[2] req_msg[2] resp_msg[3] req_msg[3] resp_msg[4] req_msg[4] resp_msg[5] req_msg[5] resp_msg[6] req_msg[6]}

set rpt_file [make_result_file check_pin_constraints3.rpt]

ppl::write_pin_constraints $rpt_file

diff_file check_pin_constraints3.rptok $rpt_file
