# test for error when multiple nets are found when automatically determining domain nets
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VDD2 -pin_pattern VDD2 -power

catch {pdngen -report_only} err
puts $err
