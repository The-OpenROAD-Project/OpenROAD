# region blocked by macro in die boundary
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef blocked_region.lef

read_def macro_not_placed.def

place_pin -pin_name clk -layer met4 -location {40 30} -force_to_die_boundary
place_pin -pin_name resp_rdy -layer met5 -location {50 15} -force_to_die_boundary
place_pins -hor_layer met3 -ver_layer met2 -random
