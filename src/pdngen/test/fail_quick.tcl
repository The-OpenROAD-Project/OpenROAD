source "helpers.tcl"

read_lef NangateOpenCellLibrary.mod.lef
read_def fail_quick/floorplan.def

catch {pdngen fail_quick/PDN.cfg -verbose} error
puts $error
