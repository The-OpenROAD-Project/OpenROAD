# Test for placing pads
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test place_pad
make_io_sites -horizontal_site IOSITE_H -vertical_site IOSITE_V -corner_site IOSITE_C -offset 15

place_pad -master PAD_H -row IO_EAST -location 500 "IO_EAST_SIDE"
place_pad -master PAD_H -row IO_WEST -location 600 "IO_WEST_SIDE"
place_pad -master PAD_V -row IO_NORTH -location 500 "IO_NORTH_SIDE"
place_pad -master PAD_V -row IO_SOUTH -location 600 "IO_SOUTH_SIDE"

place_pad -mirror -master PAD_H -row IO_EAST -location 800 "IO_EAST_SIDE_M"
place_pad -mirror -master PAD_H -row IO_WEST -location 800 "IO_WEST_SIDE_M"
place_pad -mirror -master PAD_V -row IO_NORTH -location 800 "IO_NORTH_SIDE_M"
place_pad -mirror -master PAD_V -row IO_SOUTH -location 800 "IO_SOUTH_SIDE_M"

set def_file [make_result_file "place_pad_hv.def"]
write_def $def_file
diff_files $def_file "place_pad_hv.defok"
