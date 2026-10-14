# global_placement -options: dotted keys edited into the default dict give
# the same placement as the legacy switches of simple01.
source helpers.tcl
set test_name options02
read_lef ./nangate45.lef
read_def ./simple01.def

set opts [default_options global_placement]
dict set opts nesterov.init_density_penalty 0.01
dict set opts initial.max_iter 0
global_placement -options $opts
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_files $def_file simple01.defok
