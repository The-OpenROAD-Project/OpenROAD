# global_placement: the -options dict and the legacy switches combine, and
# the switches win.  Same placement as simple01.
source helpers.tcl
set test_name options03
read_lef ./nangate45.lef
read_def ./simple01.def

global_placement -options {nesterov.init_density_penalty 0.5} \
  -init_density_penalty 0.01 -skip_initial_place
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_files $def_file simple01.defok
