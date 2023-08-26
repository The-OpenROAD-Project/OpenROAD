source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_def hybrid_cells.def
detailed_placement
check_placement -verbose 

set def_file [make_result_file hybrid_cells.def]
write_def $def_file
diff_file $def_file hybrid_cells.defok
