source $::env(SCRIPTS_DIR)/open.tcl
estimate_parasitics -placement -spef_file $::env(RESULTS_DIR)/4_cts.spef
write_verilog $::env(RESULTS_DIR)/4_cts.v
