source helpers.tcl
read_lef Nangate45.lef
read_def fence02.def
detailed_placement
check_placement

set def_file [make_result_file fence02.def]
write_def $def_file
diff_file $def_file fence02.defok
