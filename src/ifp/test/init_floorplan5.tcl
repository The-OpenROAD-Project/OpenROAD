# init_floorplan -tracks non-zero diearea origin
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "10 20 1010 1020" \
  -core_area "110 120 910 920" \
  -site FreePDK45_38x28_10R_NP_162NW_34O \
  -tracks init_floorplan2.tracks

set def_file [make_result_file init_floorplan5.def]
write_def $def_file
diff_files init_floorplan5.defok $def_file
