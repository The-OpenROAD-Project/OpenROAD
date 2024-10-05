source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def region1.def

set def_file [make_result_file region1.def]

tapcell -endcap_master sky130_fd_sc_hd__decap_3 -distance 13 -tapcell_master sky130_fd_sc_hd__tapvpwrvgnd_1

check_placement -verbose

write_def $def_file

diff_file region1.defok $def_file
