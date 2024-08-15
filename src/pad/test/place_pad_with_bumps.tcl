# Test for placing pads when bumps have been placed
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test place_pad
make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

make_io_bump_array -bump DUMMY_BUMP -origin "100 500" -rows 5 -columns 5 -pitch "55 55"
place_pad -master PADCELL_SIG_V -row IO_EAST -location 500 "IO_EAST_SIDE"
place_pad -master PADCELL_SIG_V -row IO_WEST -location 500 "IO_WEST_SIDE"

set def_file [make_result_file "place_pad_with_bumps.def"]
write_def $def_file
diff_files $def_file "place_pad_with_bumps.defok"
