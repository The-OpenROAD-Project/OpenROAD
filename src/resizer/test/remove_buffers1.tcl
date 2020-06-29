# remove_buffers in1 -> b1 -> b2 -> b3 -> out1
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

remove_buffers

set repaired_filename [file join $result_dir "remove_buffers1.def"]
write_def $repaired_filename
diff_file remove_buffers1.defok $repaired_filename
