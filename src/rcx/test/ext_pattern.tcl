source helpers.tcl

set test_nets "3 48 92 193 200 243 400 521 671"

read_lef sky130hs/sky130hs.tlef 
read_def generate_pattern.defok

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file ext_pattern.rules \
      -cc_model 12 -max_res 0 -context_depth 10 \
      -coupling_threshold 0.1

set spef_file [make_result_file ext_pattern.spef]
write_spef $spef_file -nets $test_nets

exec rm blk.totCap 

diff_files ext_pattern.spefok $spef_file
