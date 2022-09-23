source "helpers.tcl"
# overlapping cells
read_lef Nangate45/Nangate45.lef
read_def check2.def
catch {check_placement -verbose} error
puts $error

