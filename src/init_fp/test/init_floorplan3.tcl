# init_floorplan -utilization -aspect_ratio -core_space
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -utilization 30 \
  -aspect_ratio 0.5 \
  -core_space 2 \
  -tracks init_floorplan2.tracks \
  -site FreePDK45_38x28_10R_NP_162NW_34O
auto_place_pins metal1

set def_file [make_result_file init_floorplan3.def]
write_def $def_file
diff_files init_floorplan3.defok $def_file
