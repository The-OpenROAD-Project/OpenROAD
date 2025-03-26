# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -direction INPUT -region top:*
set_io_pin_constraint -direction OUTPUT -region bottom:*
catch {set_io_pin_constraint -pin_names {req_msg[14] req_msg[15] req_msg[16] req_msg[17]} -region left:*} error
puts $error
