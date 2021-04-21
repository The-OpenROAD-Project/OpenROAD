source helpers.tcl

set test_nets ""

read_lef sky130hs/sky130hs.tlef 
read_lef sky130hs/sky130hs_std_cell.lef

read_def -order_wires gcd.def

# Load via resistance info
source set_resistance.tcl

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file ext_pattern.rules \
      -max_res 0 -coupling_threshold 0.1

set spef_file [make_result_file gcd.spef] 
write_spef $spef_file -nets $test_nets

exec rm gcd.totCap

diff_files gcd.spefok $spef_file
