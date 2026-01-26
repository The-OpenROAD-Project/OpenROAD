source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef

read_verilog endcap_corners.v
link_design empty

initialize_floorplan \
  -die_area "0 0 0 100 100 100 100 0" \
  -core_area "10 10 10 80 80 80 80 90 90 90 90 10 50 10 50 20 40 20 40 10 10 10" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

set def_file [make_result_file endcap_corners.def]

place_endcaps \
  -corner TAPCELL_X1 \
  -edge_corner TAPCELL_X1 \
  -endcap TAPCELL_X1

check_placement -verbose

write_def $def_file

diff_file endcap_corners.defok $def_file
