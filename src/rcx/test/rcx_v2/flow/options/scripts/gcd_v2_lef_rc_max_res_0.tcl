set test_case gcd_v2_lef_rc_max_res_0

set test_dir ../../../../

# set model_v1 $test_dir/ext_pattern.rules

read_lef $test_dir/sky130hs/sky130hs.tlef
read_lef $test_dir/sky130hs/sky130hs_std_cell.lef
read_liberty $test_dir/sky130hs/sky130hs_tt.lib

read_def $test_dir/gcd.def

# Load via resistance info
source $test_dir/sky130hs/sky130hs.rc

define_process_corner -ext_model_index 0 X
extract_parasitics -coupling_threshold 0.1 -version 2.0 -skip_over_cell -no_merge_via_res -max_res 0 -lef_rc

write_spef $test_case.spef
