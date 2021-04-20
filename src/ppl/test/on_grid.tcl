# pins on manufacturing grid
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef

read_def on_grid.def

place_pins -hor_layer met3 -ver_layer met2

set def_file [make_result_file on_grid.def]

write_def $def_file

diff_file on_grid.defok $def_file
