read_db ~/Desktop/OpenROAD-flow-scripts/flow/results/nangate45/swerv_wrapper/base/2_2_floorplan_io.odb
read_liberty ~/Desktop/OpenROAD-flow-scripts/flow/platforms/nangate45/lib/NangateOpenCellLibrary_typical.lib
# export RTLMP_MAX_INST = 30000
# export RTLMP_MIN_INST = 5000
# export RTLMP_MAX_MACRO = 12
# export RTLMP_MIN_MACRO = 4 
# export MACRO_PLACE_HALO = 10 10
# export MACRO_PLACE_CHANNEL = 20 20
set_debug_level MPL "multilevel_autoclustering" 1
rtl_macro_placer2 -halo_height 10 -halo_width 10 -leiden_iteration 10 -max_num_macro 12 -min_num_macro 4 -max_num_inst 30000 -min_num_inst 5000 