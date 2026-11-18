# Test for placing pads
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test place_pad
make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

place_pad -row IO_EAST -location 500 "u_ddr_cke_o"
place_pad -row IO_WEST -location 600 "u_ddr_dm_0_o"
place_pad -row IO_NORTH -location 500 "u_ddr_dq_13_io"
place_pad -row IO_SOUTH -location 600 "u_ddr_dq_14_io"

set def_file [make_result_file "place_pad_no_master.def"]
write_def $def_file
diff_files $def_file "place_pad_no_master.defok"
