# The rules carry no RC data above metal5 and the design routes on metal3 only.
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def perpendicular_route_touching_bterm.def

set_extraction_rules_file rules_without_unused_layer_data.rules
extract_parasitics -max_res 0 -coupling_threshold 0.1

set spef_file [make_result_file rules_without_unused_layer_data.spef]
write_spef $spef_file
diff_files rules_without_unused_layer_data.spefok $spef_file "^\\*(DATE|VERSION)"
