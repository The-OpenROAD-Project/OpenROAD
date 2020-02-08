source helpers.tcl
read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_def nangate45-bench/gcd/gcd_replace.def
legalize_placement
set def_file [make_result_file low_util01.def]
write_def $def_file
diff_file $def_file low_util01.defok
