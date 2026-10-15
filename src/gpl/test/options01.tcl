# global_placement switches generated from PlaceOptions give the same
# placement as the older switch names of simple01.
source helpers.tcl
set test_name options01
read_lef ./nangate45.lef
read_def ./simple01.def

report_options global_placement -nesterov.init_density_penalty 0.01 -initial.max_iter 0
global_placement -nesterov.init_density_penalty 0.01 -initial.max_iter 0
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_files $def_file simple01.defok
