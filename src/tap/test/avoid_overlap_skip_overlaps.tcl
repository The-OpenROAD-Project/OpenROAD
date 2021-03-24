source "helpers.tcl"
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def gcd_sky130hs_floorplan.def

set def_file [make_result_file avoid_overlap.def]

tapcell -distance 14 -tapcell_master "sky130_fd_sc_hs__tap_1" -endcap_master "sky130_fd_sc_hs__decap_4" -skip_tapcell_in_endcap

write_def $def_file

diff_file avoid_overlap.defok $def_file
