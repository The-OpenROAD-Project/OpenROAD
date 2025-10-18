# init_floorplan called after a voltage domain is specified
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef init_floorplan_gap.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

set area {0 0 100 100}

create_voltage_domain TEMP_ANALOG -area {34 34 66 66}

# Test non positive gap
catch {
  initialize_floorplan -die_area $area \
    -core_area $area \
    -site FreePDK45_38x28_10R_NP_162NW_34O \
    -gap -1
}
catch {
  initialize_floorplan -die_area $area \
    -core_area $area \
    -site FreePDK45_38x28_10R_NP_162NW_34O \
    -gap 0
}
catch {
  make_rows -core_area $area \
    -site FreePDK45_38x28_10R_NP_162NW_34O \
    -gap -1
}
catch {
  make_rows -core_area $area \
    -site FreePDK45_38x28_10R_NP_162NW_34O \
    -gap 0
}

initialize_floorplan -die_area $area \
  -core_area $area \
  -site FreePDK45_38x28_10R_NP_162NW_34O \
  -additional_sites FreePDK45_38x28_10R_NP_162NW_34O_DoubleHeight \
  -gap 2

set def_file [make_result_file init_floorplan_gap.def]
write_def $def_file
diff_files init_floorplan_gap.defok $def_file
