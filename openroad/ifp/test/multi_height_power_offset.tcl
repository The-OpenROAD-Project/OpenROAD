source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef multi_height_power_offset.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "0 0 20 14" \
  -core_area "0 0 20 14" \
  -site FreePDK45_38x28_10R_NP_162NW_34O \
  -additional_sites DoubleExtSite

set def_file [make_result_file multi_height_power_offset.def]
write_def $def_file
diff_files multi_height_power_offset.defok $def_file
