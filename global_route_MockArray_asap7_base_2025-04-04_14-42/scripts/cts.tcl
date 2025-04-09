utl::set_metrics_stage "cts__{}"
source $::env(SCRIPTS_DIR)/load.tcl
erase_non_stage_variables cts
load_design 3_place.odb 3_place.sdc

# Clone clock tree inverters next to register loads
# so cts does not try to buffer the inverted clocks.
repair_clock_inverters

proc save_progress {stage} {
  puts "Run 'make gui_$stage.odb' to load progress snapshot"
  write_db $::env(RESULTS_DIR)/$stage.odb
  write_sdc -no_timestamp $::env(RESULTS_DIR)/$stage.sdc
}

# Run CTS
set cts_args [list \
          -sink_clustering_enable \
          -balance_levels]

append_env_var cts_args -distance_between_buffers CTS_BUF_DISTANCE 1
append_env_var cts_args -sink_clustering_size CTS_CLUSTER_SIZE 1
append_env_var cts_args -sink_clustering_max_diameter CTS_CLUSTER_DIAMETER 1

if {[env_var_exists_and_non_empty CTS_ARGS]} {
  set cts_args $::env(CTS_ARGS)
}

log_cmd clock_tree_synthesis {*}$cts_args

if {[env_var_equals CTS_SNAPSHOTS 1]} {
  save_progress 4_1_pre_repair_clock_nets
}

set_propagated_clock [all_clocks]

set_dont_use $::env(DONT_USE_CELLS)

utl::push_metrics_stage "cts__{}__pre_repair"

estimate_parasitics -placement
if { $::env(DETAILED_METRICS) } {
  report_metrics 4 "cts pre-repair"
}
utl::pop_metrics_stage

repair_clock_nets

utl::push_metrics_stage "cts__{}__post_repair"
estimate_parasitics -placement
if { $::env(DETAILED_METRICS) } {
  report_metrics 4 "cts post-repair"
}
utl::pop_metrics_stage

set_placement_padding -global \
    -left $::env(CELL_PAD_IN_SITES_DETAIL_PLACEMENT) \
    -right $::env(CELL_PAD_IN_SITES_DETAIL_PLACEMENT)
detailed_placement

estimate_parasitics -placement

if {[env_var_equals CTS_SNAPSHOTS 1]} {
  save_progress 4_1_pre_repair_hold_setup
}

if {![env_var_equals SKIP_CTS_REPAIR_TIMING 1]} {
  if {$::env(EQUIVALENCE_CHECK)} {
      write_eqy_verilog 4_before_rsz.v
  }

  repair_timing_helper

  if {$::env(EQUIVALENCE_CHECK)} {
      run_equivalence_test
  }

  set result [catch {detailed_placement} msg]
  if {$result != 0} {
    save_progress 4_1_error
    puts "Detailed placement failed in CTS: $msg"
    exit $result
  }

  check_placement -verbose
}

report_metrics 4 "cts final"

if { [env_var_exists_and_non_empty POST_CTS_TCL] } {
  source $::env(POST_CTS_TCL)
}

write_db $::env(RESULTS_DIR)/4_1_cts.odb
write_sdc -no_timestamp $::env(RESULTS_DIR)/4_cts.sdc
