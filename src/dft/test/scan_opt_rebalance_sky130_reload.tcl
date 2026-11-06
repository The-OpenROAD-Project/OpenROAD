# Reloads the design written by scan_opt_rebalance_sky130.tcl and runs
# scan_opt on it.
source "helpers.tcl"
source "scan_opt_helpers.tcl"

read_db [make_result_file scan_opt_rebalance_sky130.odb]

set lasts [scan_chain_last_cells]
scatter_insts 9
scan_opt
check_scan_chains $lasts

set def_file [make_result_file scan_opt_rebalance_sky130.def]
write_def $def_file
diff_files $def_file scan_opt_rebalance_sky130.defok
