source $::env(SCRIPTS_DIR)/load.tcl
erase_non_stage_variables generate_abstract

set stem [expr {[env_var_exists_and_non_empty ABSTRACT_SOURCE] ? $::env(ABSTRACT_SOURCE) : "6_final"}]

set result [find_sdc_file $stem.odb]
set design_stage [lindex $result 0]
set sdc_file [lindex $result 1]

log_cmd load_design $stem.odb [file tail $sdc_file]

if {$design_stage >= 6 && [file exists $::env(RESULTS_DIR)/$stem.spef]} {
  log_cmd read_spef $::env(RESULTS_DIR)/$stem.spef
} elseif {$design_stage >= 3} {
  log_cmd estimate_parasitics -placement
}

if {$design_stage >= 4} {
  set_propagated_clock [all_clocks]
}
# write_timing_model includes the source latency in the model
set_clock_latency -source 0 [all_clocks]
puts "Generating abstract views"
log_cmd write_timing_model $::env(RESULTS_DIR)/$::env(DESIGN_NAME).lib
log_cmd write_abstract_lef -bloat_occupied_layers $::env(RESULTS_DIR)/$::env(DESIGN_NAME).lef

if {[env_var_exists_and_non_empty CDL_FILES]} {
  cdl read_masters $::env(CDL_FILES)
  cdl out $::env(RESULTS_DIR)/$stem.cdl
}
