source helpers.tcl
read_lef Nangate45.lef
read_def simple05.def
detailed_placement
set def_file [make_result_file simple05.def]
write_def $def_file
diff_file $def_file simple05.defok
