source helpers.tcl
read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_def fence01.def
legalize_placement
set def_file [make_result_file fence01.def]
write_def $def_file
diff_file $def_file fence01.defok
