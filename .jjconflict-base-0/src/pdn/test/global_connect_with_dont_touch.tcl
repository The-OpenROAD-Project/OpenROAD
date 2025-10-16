# test for global connect with dont_touch, but no warnings
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VDD -pin_pattern VDD
add_global_connection -net VSS -pin_pattern VSS -ground

set_dont_touch *
global_connect
