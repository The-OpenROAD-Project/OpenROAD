# place pins at top layer
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef blocked_region.lef

read_def blocked_region.def

define_pin_shape_pattern -layer met5 -x_step 6.8 -y_step 6.8 -region { 50 50 250 250 } -size { 1.6 2.5 }
set_io_pin_constraint -pin_names {clk resp_val req_val resp_rdy reset req_rdy} -region "up:{ 170 200 250 250}"

place_pins -hor_layer met3 -ver_layer met2

set def_file [make_result_file top_layer2.def]

write_def $def_file

diff_file top_layer2.defok $def_file

exit
