source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45_data/endcaps_separate_edges.lef

read_verilog endcap_corners.v
link_design empty

initialize_floorplan \
  -die_area "0 0 100 100" \
  -core_area "10 10 90 90" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

set def_file [make_result_file endcap_separate_edges.def]

place_endcaps \
  -corner TAPCELL_X1 \
  -left_edge ENDCAP_LEFTEDGE \
  -right_edge ENDCAP_RIGHTEDGE

check_placement -verbose

write_def $def_file

diff_file endcap_separate_edges.defok $def_file
