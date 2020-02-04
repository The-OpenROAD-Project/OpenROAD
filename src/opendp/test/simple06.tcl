# illegal placement; overlapping fixed cells
source helpers.tcl
read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_def simple06.def
legalize_placement
set def_file [make_result_file simple06.def]
write_def $def_file
diff_file $def_file simple06.defok
