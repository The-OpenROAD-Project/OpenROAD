source helpers.tcl
set test_name diverge01 
read_lef ./nangate45.lef
read_def ./$test_name.def

global_placement -init_density_penalty 100 -skip_initial_place
