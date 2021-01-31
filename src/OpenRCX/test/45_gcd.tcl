source helpers.tcl

set test_nets ""

read_lef Nangate45/Nangate45.lef

read_def -order_wires 45_gcd.def

# Load via resistance info
source 45_via_resistance.tcl

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file 45_patterns.rules \
      -max_res 0 -coupling_threshold 0.1

set spef_file [make_result_file 45_gcd.spef] 
write_spef $spef_file -nets $test_nets

diff_files 45_gcd.spefok $spef_file
