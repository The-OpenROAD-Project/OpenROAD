source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_def regions3.def
detailed_placement
check_placement

set def_file [make_result_file regions3.def]
write_def $def_file
diff_file $def_file regions3.defok
