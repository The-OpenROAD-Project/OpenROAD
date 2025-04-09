utl::set_metrics_stage "detailedplace__{}"
source $::env(SCRIPTS_DIR)/load.tcl
erase_non_stage_variables place
load_design 3_4_place_resized.odb 2_floorplan.sdc

source $::env(PLATFORM_DIR)/setRC.tcl

proc do_dpl {} {
  # Only for use with hybrid rows
  if {[env_var_equals BALANCE_ROWS 1]} {
    balance_row_usage
  }
  
  set_placement_padding -global \
      -left $::env(CELL_PAD_IN_SITES_DETAIL_PLACEMENT) \
      -right $::env(CELL_PAD_IN_SITES_DETAIL_PLACEMENT)
  detailed_placement
  
  if {[env_var_equals ENABLE_DPO 1]} {
    if {[env_var_exists_and_non_empty DPO_MAX_DISPLACEMENT]} {
      improve_placement -max_displacement $::env(DPO_MAX_DISPLACEMENT)
    } else {
      improve_placement
    }
  }
  optimize_mirroring

  utl::info FLW 12 "Placement violations [check_placement -verbose]."
  
  estimate_parasitics -placement
}

set result [catch {do_dpl} errMsg]
if {$result != 0} {
  write_db $::env(RESULTS_DIR)/3_5_place_dp-failed.odb
  error $errMsg
}

report_metrics 3 "detailed place" true false

write_db $::env(RESULTS_DIR)/3_5_place_dp.odb
