source "helpers.tcl"

read_lef ../../../test/Nangate45/Nangate45.lef
read_def fail_quick/floorplan.def

catch {pdngen fail_quick/PDN.cfg -verbose} error
puts $error
