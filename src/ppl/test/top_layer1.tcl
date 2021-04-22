# place pins at top layer
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef blocked_region.lef

read_def blocked_region.def

define_pin_shape_pattern -layer met5 -x_step 3.4 -y_step 3.4 -origin { 100 100 } -size { 1.6 2.5 }
set_io_pin_constraint -pin_names {clk resp_val} -region "up:{200 200 1500 1500}"

place_pins -hor_layer met3 -ver_layer met2

set def_file [make_result_file top_layer1.def]

write_def $def_file

diff_file top_layer1.defok $def_file

exit
