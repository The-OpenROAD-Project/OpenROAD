# init_floorplan called after a voltage domain is specified
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_verilog reg2.v
link_design top

#create_voltage_domain TEMP_ANALOG -area {27 27 60 60}
#initialize_floorplan -die_area "0 0 150 150" \
#  -core_area "20 20 130 130" \
#  -site unithd
create_voltage_domain TEMP_ANALOG -area {33.58 32.64 64.86 62.56}
initialize_floorplan -die_area "0 0 155.48 146.88" \
  -core_area "18.4 16.32 137.08 130.56" \
  -site unithd
set def_file [make_result_file init_floorplan9.def]
write_def $def_file
diff_files init_floorplan9.defok $def_file
