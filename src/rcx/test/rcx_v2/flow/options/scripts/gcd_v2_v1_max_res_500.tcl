set test_case gcd_v2_v1_max_res_500

set test_dir ../../../../
# source $test_dir/helpers.tcl
set model_v1 $test_dir/ext_pattern.rules

set test_nets ""

read_lef $test_dir/sky130hs/sky130hs.tlef
read_lef $test_dir/sky130hs/sky130hs_std_cell.lef
read_liberty $test_dir/sky130hs/sky130hs_tt.lib

read_def $test_dir/gcd.def

# Load via resistance info
source $test_dir/sky130hs/sky130hs.rc

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file $model_v1 -coupling_threshold 0.1 -version 2.0 -skip_over_cell -max_res 500

set spef_file $test_case.spef
write_spef $spef_file -nets $test_nets

# read_spef $spef_file

# diff_files gcd.spefok $spef_file "^\\*(DATE|VERSION)"
