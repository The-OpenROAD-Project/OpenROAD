# Test for building bump array with a gingle pitch defined
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

# Test make_io_bump_array
make_io_bump_array -bump DUMMY_BUMP -origin "200 200" -rows 14 -columns 14 -pitch "200"

set def_file [make_result_file "bump_array_make_single_pitch.def"]
write_def $def_file
diff_files $def_file "bump_array_make_single_pitch.defok"
