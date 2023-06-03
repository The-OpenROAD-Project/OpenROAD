source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_def multi_height_rows.def
detailed_placement
filler_placement FILL*
check_placement

set def_file [make_result_file multi_height_rows.def]
write_def $def_file
diff_file $def_file multi_height_rows.defok
