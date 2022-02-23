source helpers.tcl
set test_name simple01-skip-io
read_lef ./nangate45.lef
read_def ./$test_name.def

global_placement -skip_io
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
