# init_floorplan called after a voltage domain is specified
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef init_floorplan_dbl_row.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
create_voltage_domain TEMP_ANALOG -area {34 34 66 66}
initialize_floorplan -die_area {0 0 100 100} \
  -core_area {0 0 100 100} \
  -site FreePDK45_38x28_10R_NP_162NW_34O \
  -additional_sites FreePDK45_38x28_10R_NP_162NW_34O_DoubleHeight

set def_file [make_result_file init_floorplan_dbl_row.def]
write_def $def_file
diff_files init_floorplan_dbl_row.defok $def_file
