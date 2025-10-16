# make_tracks -add
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "0 0 1000 1000" \
  -core_area "100 100 900 900" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

make_tracks metal1 -x_offset 0.1 -x_pitch 0.2 -y_offset 0.1 -y_pitch 0.2
make_tracks metal1 -x_offset 0.15 -x_pitch 0.2 -y_offset 0.15 -y_pitch 0.2

set def_file [make_result_file make_tracks5.def]
write_def $def_file
diff_files make_tracks5.defok $def_file
