# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def large_groups1.def

set group1 {}
for {set i 0} {$i < 300} {incr i} {
 lappend group1 "pin$i"
}

set group2 {}
for {set i 300} {$i < 371} {incr i} {
 lappend group2 "pin$i"
}

set_io_pin_constraint -region top:* -group -order -pin_names $group1

set_io_pin_constraint -region top:* -group -order -pin_names $group2

catch {place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 15 -min_distance 0.12} error
puts $error
