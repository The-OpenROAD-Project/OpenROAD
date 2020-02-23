# 10 inst placement legal (should not move anything)
source helpers.tcl
read_lef NangateOpenCellLibrary.lef
read_def simple04.def
legalize_placement
set def_file [make_result_file simple04.def]
write_def $def_file
diff_file $def_file simple04.defok
