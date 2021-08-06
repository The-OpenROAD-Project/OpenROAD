if {![info exists standalone] || $standalone} {
  # Read lef
  read_lef $env(TECH_LEF)
  read_lef $env(SC_LEF)
  if {[info exist env(ADDITIONAL_LEFS)]} {
    foreach lef $env(ADDITIONAL_LEFS) {
      read_lef $lef
    }
  }

  # Read liberty files
  foreach libFile $env(LIB_FILES) {
    read_liberty $libFile
  }

  # Read design files
  # Read SDC and derating files
  read_def $env(RESULTS_DIR)/4_cts.def
  read_sdc $env(RESULTS_DIR)/2_floorplan.sdc
  if [file exists $env(PLATFORM_DIR)/derate_final.tcl] {
    source $env(PLATFORM_DIR)/derate_final.tcl
    puts "derate_final.tcl sourced"
  }
} else {
  puts "Starting global routing"
}

if {[info exist env(PRE_GLOBAL_ROUTE)]} {
  source $env(PRE_GLOBAL_ROUTE)
}

if {[info exist env(FASTROUTE_TCL)]} {
  source $env(FASTROUTE_TCL)
} else {
  set_global_routing_layer_adjustment $env(MIN_ROUTING_LAYER)-$env(MAX_ROUTING_LAYER) 0.5
  set_routing_layers -signal $env(MIN_ROUTING_LAYER)-$env(MAX_ROUTING_LAYER)
  set_macro_extension 2
}

global_route -guide_file $env(RESULTS_DIR)/route.guide \
               -congestion_iterations 100 \
               -verbose 2

# Set res and cap
if [file exists $env(PLATFORM_DIR)/setRC.tcl] {
  source $env(PLATFORM_DIR)/setRC.tcl
}

set_propagated_clock [all_clocks]
estimate_parasitics -global_routing

source $env(SCRIPTS_DIR)/report_metrics.tcl
report_metrics "global route"

puts "\n=========================================================================="
puts "check_antennas"
puts "--------------------------------------------------------------------------"
check_antennas -report_file $env(REPORTS_DIR)/antenna.log -report_violating_nets

# Write SDC to results with updated clock periods that are just failing.
# Use make target update_sdc_clock to install the updated sdc.
source [file join $env(SCRIPTS_DIR) "write_ref_sdc.tcl"]
