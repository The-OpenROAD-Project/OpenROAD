source $::env(SCRIPTS_DIR)/util.tcl
# Read liberty files
source $::env(SCRIPTS_DIR)/read_liberty.tcl

# Read def
if {[env_var_exists_and_non_empty DEF_FILE]} {
    # Read lef
    log_cmd read_lef $::env(TECH_LEF)
    log_cmd read_lef $::env(SC_LEF)
    if {[env_var_exists_and_non_empty ADDITIONAL_LEFS]} {
      foreach lef $::env(ADDITIONAL_LEFS) {
        log_cmd read_lef $lef
      }
    }
    set input_file $::env(DEF_FILE)
    log_cmd read_def $input_file
} else {
    set input_file $::env(ODB_FILE)
    log_cmd read_db $input_file
}

proc read_timing {input_file} {
  set result [find_sdc_file $input_file]
  set design_stage [lindex $result 0]
  set sdc_file [lindex $result 1]

  if {$sdc_file == ""} {
    set sdc_file $::env(SDC_FILE)
  }
  log_cmd read_sdc $sdc_file
  if [file exists $::env(PLATFORM_DIR)/derate.tcl] {
    source $::env(PLATFORM_DIR)/derate.tcl
  }
  
  source $::env(PLATFORM_DIR)/setRC.tcl
  if {$design_stage >= 4} {
    # CTS has run, so propagate clocks
    set_propagated_clock [all_clocks]
  }
  
  if {$design_stage >= 6 && [file exist $::env(RESULTS_DIR)/6_final.spef]} {
    log_cmd read_spef $::env(RESULTS_DIR)/6_final.spef
  } elseif {$design_stage >= 5} {
    if { [grt::have_routes] } {
      log_cmd estimate_parasitics -global_routing
    } else {
      puts "No global routing results available, skipping estimate_parasitics"
      puts "Load $::global_route_congestion_report for details"
    }
  } elseif {$design_stage >= 3} {
    log_cmd estimate_parasitics -placement
  }

  puts -nonewline "Populating timing paths..."
  # Warm up OpenSTA, so clicking on timing related buttons reacts faster
  set _tmp [find_timing_paths]
  puts "OK"
}

if {[ord::openroad_gui_compiled]} {
  set db_basename [file rootname [file tail $input_file]]
  gui::set_title "OpenROAD - $::env(PLATFORM)/$::env(DESIGN_NICKNAME)/$::env(FLOW_VARIANT) - ${db_basename}"
}

if {[env_var_equals GUI_TIMING 1]} {
  puts "GUI_TIMING=1 reading timing, takes a little while for large designs..."
  read_timing $input_file
  # if {[gui::enabled]} {
  #   gui::select_chart "Endpoint Slack"
  #   log_cmd gui::update_timing_report
  # }
}

fast_route
