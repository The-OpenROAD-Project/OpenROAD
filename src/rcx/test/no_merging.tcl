# Test if the RC topology is correctly generated when no merging
# mechanism is used.
source helpers.tcl

read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_liberty sky130hs/sky130hs_tt.lib

read_def gcd.def

# Load via resistance info
source sky130hs/sky130hs.rc

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file ext_pattern.rules -no_merge_via_res -max_res 0.0

set spef_file [make_result_file no_merging.spef]
write_spef $spef_file

diff_files no_merging.spefok $spef_file "^\\*(DATE|VERSION)"
