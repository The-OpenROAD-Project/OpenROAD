source helpers.tcl
set test_name simple01
read_lef ./nangate45.lef
read_def ./$test_name.def

global_placement -init_density_penalty 0.01 -skip_initial_place -density uniform
set def_file [make_result_file $test_name-uniform.def]
write_def $def_file
diff_file $def_file $test_name-uniform.defok
