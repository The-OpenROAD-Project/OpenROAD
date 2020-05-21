source helpers.tcl
read_lef Nangate45.lef
read_def fence03.def
detailed_placement
set def_file [make_result_file fence03.def]
write_def $def_file
diff_file $def_file fence03.defok
