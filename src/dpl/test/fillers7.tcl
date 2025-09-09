# filler_placement for simple01
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def simple01.def
detailed_placement
filler_placement -prefix "NANGATEFILL_" FILL*
check_placement

set def_file [make_result_file fillers7.def]
write_def $def_file
diff_file $def_file fillers7.defok
