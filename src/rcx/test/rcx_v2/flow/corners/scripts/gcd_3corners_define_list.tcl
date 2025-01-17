set test_case gcd_3corners_define_list
set test_dir ../../../../
set model_v2 ../../../data/3corners.rcx.model

read_lef $test_dir/sky130hs/sky130hs.tlef
read_lef $test_dir/sky130hs/sky130hs_std_cell.lef
read_liberty $test_dir/sky130hs/sky130hs_tt.lib

read_def $test_dir/gcd.def

source $test_dir/sky130hs/sky130hs.rc

get_model_corners -ext_model_file $model_v2
define_rcx_corners -corner_list "min typ max"

extract_parasitics -ext_model_file $model_v2 -max_res 0 -coupling_threshold 0.001 -version 2.0 -skip_over_cell

write_spef $test_case.spef
