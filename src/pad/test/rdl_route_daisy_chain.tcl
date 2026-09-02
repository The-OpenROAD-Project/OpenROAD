# Test for RDL router without 45*
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 35

place_pad -master PADCELL_SIG_V -row IO_SOUTH -location 205.0 u_ddr_dm_1_o

make_io_bump_array -bump DUMMY_BUMP -origin "210.0 215.0" -pitch "160 160" -rows 17 -columns 17
# Create a chain of 3 bumps to route to pad

# Close to pad
assign_io_bump -net p_ddr_dm_1_o BUMP_0_0

# Far from pad, but close to each other
assign_io_bump -net p_ddr_dm_1_o BUMP_11_12
assign_io_bump -net p_ddr_dm_1_o BUMP_12_12

rdl_route -layer metal10 -width 4 -spacing 4 "p_ddr_dm_1_o"

set def_file [make_result_file "rdl_route_daisy_chain.def"]
write_def $def_file
diff_files $def_file "rdl_route_daisy_chain.defok"
