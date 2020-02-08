source helpers.tcl
read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_def fragmented_row03.def
legalize_placement
set def_file [make_result_file fragmented_row03.def]
write_def $def_file
diff_file $def_file fragmented_row03.defok
