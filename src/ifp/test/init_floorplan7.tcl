# init_floorplan called twice for some stupid reason
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "0 0 100 100" \
  -core_area "10 10 90 90" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
initialize_floorplan -die_area "10 10 110 110" \
  -core_area "20 20 80 80" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

set def_file [make_result_file init_floorplan7.def]
write_def $def_file
diff_files init_floorplan7.defok $def_file
