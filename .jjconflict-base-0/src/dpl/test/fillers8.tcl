# remove_filers
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def fillers8.def

catch { detailed_placement } msg
puts $msg

remove_fillers
check_placement

set def_file [make_result_file fillers8.def]
write_def $def_file
diff_file $def_file fillers8.defok
