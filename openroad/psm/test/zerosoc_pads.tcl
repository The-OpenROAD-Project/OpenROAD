source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130hd_data/ram/sky130_sram_2kbyte_1rw1r_32x512_8.lef
read_lef sky130hd_data/io/sky130_dummy_io.lef

read_def sky130hd_data/zerosoc_pads.def

read_liberty sky130hd/sky130hd_tt.lib
read_liberty sky130hd_data/ram/sky130_sram_2kbyte_1rw1r_32x512_8_TT_1p8V_25C.lib
read_liberty sky130hd_data/io/sky130_dummy_io.lib

read_sdc sky130hd_data/zerosoc_pads.sdc

source sky130hd/sky130hd.rc

analyze_power_grid -net vdd
