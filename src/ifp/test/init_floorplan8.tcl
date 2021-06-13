# init_floorplan called after a voltage domain is specified
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
create_voltage_domain TEMP_ANALOG -area {27 27 60 60}
initialize_floorplan -die_area "0 0 150 150" \
  -core_area "20 20 130 130" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
set def_file [make_result_file init_floorplan8.def]
write_def $def_file
diff_files init_floorplan8.defok $def_file

