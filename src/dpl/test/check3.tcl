source "helpers.tcl"
# abutting cells / overlap padded
read_lef Nangate45/Nangate45.lef
read_def check3.def
set_placement_padding -global -left 1 -right 1
catch {check_placement -verbose} error
puts $error

