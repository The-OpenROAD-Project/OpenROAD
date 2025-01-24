source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_verilog multi_height1.v 
link_design top
initialize_floorplan -utilization 30 -core_space 0.0 -aspect_ratio 0.5 \
  -site FreePDK45_38x28_10R_NP_162NW_34O

set def_file [make_result_file multi_height1.def]
write_def $def_file
diff_files multi_height1.defok $def_file
