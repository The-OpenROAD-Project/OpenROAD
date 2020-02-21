# 1 inst -pad_right
source helpers.tcl
read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_def simple03.def
legalize_placement -pad_right 5

set def_file [make_result_file pad02.def]
write_def $def_file
diff_file $def_file pad02.defok
