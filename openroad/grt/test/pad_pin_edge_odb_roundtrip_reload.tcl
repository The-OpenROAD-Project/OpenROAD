# Estimate parasitics from the stored guides with new access points.
set test_name pad_pin_edge_odb_roundtrip
source "helpers.tcl"
read_db [make_result_file "${test_name}.odb"]
set_routing_layers -signal Metal2-Metal5
pin_access -verbose 0
estimate_parasitics -global_routing
