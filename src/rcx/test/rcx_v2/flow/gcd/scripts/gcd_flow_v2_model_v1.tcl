set test_case gcd_flow_v2_model_v1

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
extract_parasitics -ext_model_file $model_v1 -max_res 0 -coupling_threshold 0.1 -version 2.0 -skip_over_cell

set spef_file $test_case.spef
write_spef $spef_file -nets $test_nets
