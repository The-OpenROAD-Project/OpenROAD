# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -direction INPUT -region right:*
set_io_pin_constraint -pin_names { resp_val req_rdy } -region bottom:4-18
set_io_pin_constraint -pin_names { resp_msg[15] resp_msg[14] } -region top:7-21

set rpt_file [make_result_file check_pin_constraints1.rpt]

ppl::write_pin_constraints $rpt_file

diff_file check_pin_constraints1.rptok $rpt_file
