# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_io_pin_constraint -pin_names {resp_val} -region bottom:*
set_io_pin_constraint -pin_names {clk} -region top:*
set_io_pin_constraint -pin_names {reset} -region left:*
set_io_pin_constraint -pin_names {resp_rdy} -region right:*
place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12 -annealing

set def_file [make_result_file annealing_constraint8.def]

write_def $def_file

diff_file annealing_constraint8.defok $def_file