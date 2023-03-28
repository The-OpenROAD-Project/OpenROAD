# make_tracks -add
source "helpers.tcl"
read_lef asap7_data/asap7.lef
read_def asap7_data/asap7.def

make_tracks 

set def_file [make_result_file make_tracks7.def]
write_def $def_file
diff_files make_tracks7.defok $def_file
