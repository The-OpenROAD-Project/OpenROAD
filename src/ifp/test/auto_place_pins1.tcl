# auto_place_pins
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan -die_area "0 0 1000 1000" \
  -core_area "100 100 900 900" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
auto_place_pins metal1

set def_file [make_result_file auto_place_pins1.def]
write_def $def_file
diff_files auto_place_pins1.defok $def_file
