# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd_obs.def

define_pin_shape_pattern -layer metal7 -x_step 5 -y_step 3 -region [ord::get_core_area] -size { 1.6 2.5 } -pin_keepout 1

set_io_pin_constraint -pin_names * -region "up:*"

place_pins -hor_layers metal3 -ver_layers metal2 -random

set def_file [make_result_file top_layer7.def]

write_def $def_file

diff_file top_layer7.defok $def_file
