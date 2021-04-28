# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

define_pin_shape_pattern -layer metal10 -x_step 4.8 -y_step 4.8 -region { 0.095 0.07 90 90 } -size { 1.6 2.5 }
catch {set_io_pin_constraint -pin_names {qq} -region "up:{70 50 95 100}"} error
puts $error