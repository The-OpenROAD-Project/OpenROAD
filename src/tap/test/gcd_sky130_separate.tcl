source "helpers.tcl"
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def gcd_sky130hs_floorplan.def

set def_file [make_result_file gcd_sky130_seperate.def]

cut_rows -endcap_master sky130_fd_sc_hs__decap_4

place_endcaps -endcap_vertical sky130_fd_sc_hs__decap_4

place_tapcells -distance 15 -master sky130_fd_sc_hs__tap_1

write_def $def_file

diff_file gcd_sky130.defok $def_file
