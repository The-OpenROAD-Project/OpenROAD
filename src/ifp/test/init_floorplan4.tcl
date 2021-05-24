# init_floorplan -core_area non-multiples of site size
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "0 0 100 100" \
  -core_area "11 11 90 90" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

set def_file [make_result_file init_floorplan4.def]
write_def $def_file
diff_files init_floorplan4.defok $def_file
