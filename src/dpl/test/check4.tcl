source "helpers.tcl"
# off grid fixed inst
read_lef Nangate45/Nangate45.lef
read_def check4.def
catch {check_placement -verbose} error
puts $error

