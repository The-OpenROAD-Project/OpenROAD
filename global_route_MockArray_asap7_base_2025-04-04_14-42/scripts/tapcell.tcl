source $::env(SCRIPTS_DIR)/load.tcl
erase_non_stage_variables floorplan

load_design 2_2_floorplan_macro.odb 2_1_floorplan.sdc

if {[env_var_exists_and_non_empty TAPCELL_TCL]} {
    source $::env(TAPCELL_TCL)
} else {
    cut_rows
}

write_db $::env(RESULTS_DIR)/2_3_floorplan_tapcell.odb
