# The driver pin is only touched by the route, with no route vertex inside it.
# The extracted net must still start at the driver and list it in *CONN.
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def perpendicular_route_touching_iterm.def

define_process_corner -ext_model_index 0 X
set_extraction_rules_file 45_patterns.rules
extract_parasitics -max_res 0 -coupling_threshold 0.1

set spef_file [make_result_file perpendicular_route_touching_iterm.spef]
write_spef $spef_file

diff_files perpendicular_route_touching_iterm.spefok $spef_file "^\\*(DATE|VERSION)"
