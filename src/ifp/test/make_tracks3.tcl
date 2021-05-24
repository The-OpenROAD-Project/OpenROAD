# make_tracks non-zero diearea origin
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "10 20 101 102" \
  -core_area "11 12 91 92" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

make_tracks metal1 -x_offset 0.1 -x_pitch 0.2 -y_offset 0.1 -y_pitch 0.2
make_tracks metal2 -x_offset 0.1 -x_pitch 0.2 -y_offset 0.1 -y_pitch 0.2

set def_file [make_result_file make_tracks3.def]
write_def $def_file
diff_files make_tracks3.defok $def_file
