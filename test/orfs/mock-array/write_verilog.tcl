source $::env(SCRIPTS_DIR)/open.tcl
log_cmd write_verilog -remove_cells $::env(ASAP7_REMOVE_CELLS) $::env(OUTPUT)
