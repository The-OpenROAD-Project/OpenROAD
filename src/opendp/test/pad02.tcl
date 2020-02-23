# set_padding -global -right
source helpers.tcl
read_lef NangateOpenCellLibrary.lef
read_def simple03.def
set_padding -global -right 5

set def_file [make_result_file pad02.def]
write_def $def_file
diff_file $def_file pad02.defok
