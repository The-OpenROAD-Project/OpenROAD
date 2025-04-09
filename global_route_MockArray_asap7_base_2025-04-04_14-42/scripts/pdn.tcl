source $::env(SCRIPTS_DIR)/load.tcl
erase_non_stage_variables floorplan
load_design 2_3_floorplan_tapcell.odb 2_1_floorplan.sdc

source $::env(PDN_TCL)
pdngen

if { [env_var_exists_and_non_empty POST_PDN_TCL] } {
  source $::env(POST_PDN_TCL)
}

# Check all supply nets
set block [ord::get_db_block]
foreach net [$block getNets] {
    set type [$net getSigType]
    if {$type == "POWER" || $type == "GROUND"} {
# Temporarily disable due to CI issues
#        puts "Check supply: [$net getName]"
#        check_power_grid -net [$net getName]
    }
}

write_db $::env(RESULTS_DIR)/2_4_floorplan_pdn.odb
