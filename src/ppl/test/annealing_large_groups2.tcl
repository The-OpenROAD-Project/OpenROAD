# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def large_groups1.def

set group {}
for {set i 0} {$i < 400} {incr i} {
 lappend group "pin$i"
}

set_io_pin_constraint -group -order -pin_names $group

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12 -annealing

set def_file [make_result_file annealing_large_groups2.def]

write_def $def_file

diff_file annealing_large_groups2.defok $def_file