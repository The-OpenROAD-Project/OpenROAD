# 1 inst -pad_left
source helpers.tcl
read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_def simple01.def
legalize_placement -pad_left 5

set def_file [make_result_file pad01.def]
write_def $def_file
diff_file $def_file pad01.defok
