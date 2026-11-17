# Test for removing a bump array
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test remove_io_bump_array
make_io_bump_array -bump DUMMY_BUMP -origin "200 200" -rows 14 -columns 14 -pitch "200 200"
remove_io_bump_array -bump DUMMY_BUMP

set def_file [make_result_file "bump_array_remove.def"]
write_def $def_file
diff_files $def_file "bump_array_remove.defok"
