# global_placement -options: a nested dict gives the same placement as the
# legacy switches of simple01.
source helpers.tcl
set test_name options01
read_lef ./nangate45.lef
read_def ./simple01.def

set opts {
  initial { max_iter 0 }
  nesterov { init_density_penalty 0.01 }
}
report_options global_placement -options $opts
global_placement -options $opts
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_files $def_file simple01.defok
