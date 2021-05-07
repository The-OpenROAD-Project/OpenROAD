# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

define_pin_shape_pattern -layer metal10 -x_step 4.8 -y_step 4.8 -region * -size { 1.6 2.5 }
set_io_pin_constraint -pin_names * -region "up:{0.095 0.07 90 90}"
place_pins -hor_layers metal3 -ver_layers metal2 -random

set def_file [make_result_file random6.def]

write_def $def_file

diff_file random6.defok $def_file
