# init_floorplan ensure only one set of tracks appear after initialize_floorplan
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "0 0 1000 1000" \
  -core_area "110 110 900 900" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
make_tracks

# Repeat with diffent size
initialize_floorplan -die_area "0 0 500 500" \
  -core_area "110 110 490 490" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
make_tracks

set def_file [make_result_file init_floorplan11.def]
write_def $def_file
diff_files init_floorplan11.defok $def_file
