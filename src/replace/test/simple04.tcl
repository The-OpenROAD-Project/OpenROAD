source helpers.tcl
set test_name simple04
read_lef ./nangate45.lef
read_def ./$test_name.def

global_placement -disable_routability_driven -disable_timing_driven 
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
