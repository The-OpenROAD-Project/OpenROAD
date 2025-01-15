set test_case gcd_corners_2
set test_dir ../../../../
set model_v2 ../../../data/3corners.rcx.model

read_lef $test_dir/sky130hs/sky130hs.tlef
read_lef $test_dir/sky130hs/sky130hs_std_cell.lef
read_liberty $test_dir/sky130hs/sky130hs_tt.lib

read_def $test_dir/gcd.def

# Load via resistance info
source $test_dir/sky130hs/sky130hs.rc

define_process_corner -ext_model_index 2 MAX1
extract_parasitics -ext_model_file $model_v2 -max_res 0 -coupling_threshold 0.001 -version 2.0 -skip_over_cell

write_spef $test_case.spef
