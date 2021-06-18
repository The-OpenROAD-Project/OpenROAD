source helpers.tcl
set test_name density01 
read_lef ./nangate45.lef
read_def ./$test_name.def

# test scripts
foreach val {0 1 2} {
  set density [get_global_placement_uniform_density -pad_left $val -pad_right $val]
  puts "pad : $val -> [format {%.3f} $density]"
}

# in-memory test after retrieving the all required densities.
global_placement -init_density_penalty 0.01 -skip_initial_place

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
