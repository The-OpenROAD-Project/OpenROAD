# region blocked by macro in die boundary
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef blocked_region.lef

read_def blocked_region.def

place_pins -hor_layer met3 -ver_layer met2 -random

set def_file [make_result_file blocked_region.def]

write_def $def_file

diff_file blocked_region.defok $def_file