# test multiple calls of place_pins after defining io constraints
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -direction INPUT -region top:*
place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file1 [make_result_file add_constraint18.def1]

write_def $def_file1

clear_io_pin_constraints

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.24

set def_file2 [make_result_file add_constraint18.def2]

write_def $def_file2

diff_file add_constraint18_1.defok $def_file1
diff_file add_constraint18_2.defok $def_file2
