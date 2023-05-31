# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

catch {set_io_pin_constraint -region top:* -order -pin_names {resp_msg[3] resp_msg[2] resp_msg[14] req_val}} error
puts $error
