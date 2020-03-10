source helpers.tcl
read_lef multi_height_tech.lef
read_lef multi_height_tech_cells.lef
read_def multi_height02.def
detailed_placement
check_placement

set def_file [make_result_file multi_height02.def]
write_def $def_file
diff_file multi_height02.defok $def_file
