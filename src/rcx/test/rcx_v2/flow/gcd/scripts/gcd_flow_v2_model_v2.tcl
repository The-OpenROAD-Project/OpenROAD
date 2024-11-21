set test_case gcd_flow_v2_model_v2
set test_dir ../../../../
set model_v2 /home/dimitris-ic/icb/icbrcx/src/rcx/test/via_res/corners.0322/data.m3/3corners.rcx.model

set test_nets ""

read_lef $test_dir/sky130hs/sky130hs.tlef
read_lef $test_dir/sky130hs/sky130hs_std_cell.lef
read_liberty $test_dir/sky130hs/sky130hs_tt.lib

read_def $test_dir/gcd.def

# Load via resistance info
source $test_dir/sky130hs/sky130hs.rc

# define_process_corner -ext_model_index 0 X
# define_process_corner -ext_model_index 1 Y
# define_process_corner -ext_model_index 2 Z
get_model_corners -ext_model_file $model_v2
define_rcx_corners -corner_list "max typ min"

extract_parasitics -ext_model_file $model_v2 -max_res 0 -coupling_threshold 0.001 -version 2.0 -skip_over_cell -dbg 2
# different test: extract_parasitics -ext_model_file $model_v2 -max_res 0 -coupling_threshold 0.1 -version 2.0
# extract_parasitics -ext_model_file $model_v2 -max_res 0 -version 2.0

set spef_file $test_case.spef
write_spef $spef_file -nets $test_nets

# read_spef $spef_file

# diff_files gcd.spefok $spef_file "^\\*(DATE|VERSION)"
