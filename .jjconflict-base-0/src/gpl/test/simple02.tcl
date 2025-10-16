source helpers.tcl
set test_name simple02
read_lef ./nangate45.lef
read_def ./$test_name.def

global_placement -density 0.6 -init_density_penalty 0.01 -skip_initial_place
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
