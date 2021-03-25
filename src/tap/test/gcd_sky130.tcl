source "helpers.tcl"
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def gcd_sky130hs_floorplan.def

set def_file [make_result_file gcd_sky130.def]

tapcell -distance 15 -tapcell_master "sky130_fd_sc_hs__tap_1" -endcap_master "sky130_fd_sc_hs__decap_4"

write_def $def_file

diff_file gcd_sky130.defok $def_file
