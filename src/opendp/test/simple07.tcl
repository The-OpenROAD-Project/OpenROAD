source helpers.tcl
read_lef NangateOpenCellLibrary.lef
read_def simple07.def
legalize_placement
set def_file [make_result_file simple07.def]
write_def $def_file
diff_file $def_file simple07.defok
