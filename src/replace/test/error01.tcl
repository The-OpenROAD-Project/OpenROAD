source helpers.tcl
set test_name error01 
read_lef ./nangate45.lef
read_def ./$test_name.def

catch {global_placement -init_density_penalty 0.01 \
         -skip_initial_place -density 0.001} error
puts $error
