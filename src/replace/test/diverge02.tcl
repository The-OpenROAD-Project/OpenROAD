source helpers.tcl
set test_name diverge02 
read_lef ./nangate45.lef
read_def ./$test_name.def

catch {global_placement -init_density_penalty 1 \
         -skip_initial_place \
         -disable_timing_driven} error
puts $error
