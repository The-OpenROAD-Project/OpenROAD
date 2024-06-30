# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -region top:10-30 -pin_names {*}

catch {place_pins -hor_layers metal3 -ver_layers metal2} error

puts $error
