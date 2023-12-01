# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

catch {set_io_pin_constraint -direction INPUT -region top:* -group -order} error
puts $error
