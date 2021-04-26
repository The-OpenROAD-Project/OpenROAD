# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

define_pin_shape_pattern -layer metal10 -x_step 4.8 -y_step 4.8 -origin { 0.095 0.07 } -size { 1.6 2.5 }
set_io_pin_constraint -pin_names {clk resp_val req_val resp_rdy reset req_rdy} -region "up:{70 50 95 100}"

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12

set def_file [make_result_file top_layer3.def]

write_def $def_file

diff_file top_layer3.defok $def_file