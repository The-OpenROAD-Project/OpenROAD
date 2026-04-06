source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def row_boundary_filter.def
detailed_placement
check_placement

set def_file [make_result_file row_boundary_filter.def]
write_def $def_file
diff_file $def_file row_boundary_filter.defok
