source "helpers.tcl"
# check_placement without detailed_placement
read_lef Nangate45/Nangate45.lef
read_def simple01.def
catch {check_placement -verbose} error
puts $error

