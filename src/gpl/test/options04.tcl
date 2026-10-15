# -options takes the whole struct as a nested dict (dotted keys work too)
# and combines with the switches, later ones overriding.  Same placement
# as simple01.
source helpers.tcl
set test_name options04
read_lef ./nangate45.lef
read_def ./simple01.def

set opts [default_options global_placement]
dict set opts initial.max_iter 0
report_options global_placement -options $opts -nesterov.init_density_penalty 0.01
global_placement -options {nesterov {init_density_penalty 0.5}} \
  -options $opts -nesterov.init_density_penalty 0.01
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_files $def_file simple01.defok
