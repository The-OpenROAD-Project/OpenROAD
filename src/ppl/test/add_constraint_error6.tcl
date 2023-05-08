# place pins at top layer
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef blocked_region.lef

read_def blocked_region.def

catch {set_io_pin_constraint -pin_names {clk resp_val req_val resp_rdy reset req_rdy} -region "up:*"} error
puts $error
