set test_case ext_patterns_flow_v2_model_v2
set test_dir ../../../../
set model_v2 /home/dimitris-ic/icb/icbrcx/src/rcx/test/via_res/corners.0322/data.m3/3corners.rcx.model

read_lef $test_dir/sky130hs/sky130hs.tlef
read_def $test_dir/generate_pattern.defok

get_model_corners -ext_model_file $model_v2
define_rcx_corners -corner_list "max typ min"
extract_parasitics -ext_model_file $model_v2 -cc_model 12 -max_res 0 -context_depth 10 -coupling_threshold 0.1 -version 2.0 -dbg 1

set test_nets "3 48 92 193 200 243 400 521 671"
write_spef $test_case.spef -nets $test_nets
# write_spef $test_case.spef
