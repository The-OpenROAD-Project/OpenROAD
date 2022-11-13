# pin length min area error
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def on_grid.def

set_pin_length -hor_length 0.1 -ver_length 0.1

catch {place_pins -hor_layer met3 -ver_layer met2} error
puts $error