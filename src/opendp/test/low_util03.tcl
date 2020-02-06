source helpers.tcl
read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_def nangate45-bench/ibex_core/ibex_core_replace.def
legalize_placement
set def_file [make_result_file low_util03.def]
write_def $def_file
diff_file $def_file low_util03.defok
