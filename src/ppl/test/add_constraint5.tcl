# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -direction INPUT -region right:*
set_io_pin_constraint -direction OUTPUT -region left:*
catch {set_io_pin_constraint -pin_names {resp_val resp_rdy req_rdy req_val} -region bottom:*} error

puts $error
