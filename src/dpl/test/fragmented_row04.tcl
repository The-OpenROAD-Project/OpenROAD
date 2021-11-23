# rows with middle cut out and 1 inst placed in cutout
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def fragmented_row04.def
catch {check_placement -verbose} error
puts $error
detailed_placement
check_placement -verbose


set def_file [make_result_file fragmented_row04.def]
write_def $def_file
diff_file fragmented_row04.defok $def_file
