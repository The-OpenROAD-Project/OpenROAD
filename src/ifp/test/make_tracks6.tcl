# make_tracks without liberty
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "0 0 1000 1000" \
  -core_area "100 100 900 900" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

make_tracks

set def_file [make_result_file make_tracks6.def]
write_def $def_file
diff_files make_tracks6.defok $def_file
