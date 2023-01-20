# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

catch {set_io_pin_constraint -direction INPUT -region top:* -mirrored_pins {resp_msg[0] req_msg[0] resp_msg[1] req_msg[1] resp_msg[2] req_msg[2] resp_msg[3]}} error
puts $error
