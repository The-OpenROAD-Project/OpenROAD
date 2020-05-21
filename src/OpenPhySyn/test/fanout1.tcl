source helpers.tcl
psn::set_log_pattern "\[%^%l%$\] %v"
puts [psn::has_transform buffer_fanout]
read_lef ../test/data/libraries/Nangate45/NangateOpenCellLibrary.mod.lef
read_def fanout1.def
read_liberty ../test/data/libraries/Nangate45/NangateOpenCellLibrary_typical.lib

optimize_fanout -max_fanout 2 -buffer_cell BUF_X2
set def_file [make_result_file fanout1.def]
write_def $def_file
diff_file $def_file fanout1.defok