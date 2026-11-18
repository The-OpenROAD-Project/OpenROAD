source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130hd_data/ram/sky130_sram_2kbyte_1rw1r_32x512_8.lef
read_lef sky130hd_data/io/sky130_dummy_io.lef

read_def sky130hd_data/zerosoc_pads.def

source sky130hd/sky130hd.rc

check_power_grid -net vdd
