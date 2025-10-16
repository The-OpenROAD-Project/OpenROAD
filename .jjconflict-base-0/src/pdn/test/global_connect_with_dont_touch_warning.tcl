# test for global connect with dont_touch, but with warnings
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

set_dont_touch *

add_global_connection -net VDD -pin_pattern VDD -power
