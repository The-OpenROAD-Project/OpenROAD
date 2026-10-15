# Generated switches, their older aliases and unique prefixes mix freely.
# Same placement as simple01.
source helpers.tcl
set test_name options02
read_lef ./nangate45.lef
read_def ./simple01.def

global_placement -init_density_penalty 0.01 -initial.max_it 0
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_files $def_file simple01.defok
