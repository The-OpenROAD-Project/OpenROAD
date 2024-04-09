# remove_filers
source "helpers.tcl"
read_lef fillers9.lef
read_def fillers9.def

filler_placement FILL*

check_placement

set def_file [make_result_file fillers9.def]
write_def $def_file
diff_file $def_file fillers9.defok
