source helpers.tcl
read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_def simple01.def
legalize_placement
set def_file [make_result_file simple01.def]
write_def $def_file
diff_file $def_file simple01.defok
