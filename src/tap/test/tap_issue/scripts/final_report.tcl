if {![info exists standalone] || $standalone} {
  # Read lef
  read_lef $::env(TECH_LEF)
  read_lef $::env(SC_LEF)
  if {[info exist ::env(ADDITIONAL_LEFS)]} {
    foreach lef $::env(ADDITIONAL_LEFS) {
      read_lef $lef
    }
  }

  # Read liberty files
  foreach libFile $::env(LIB_FILES) {
    read_liberty $libFile
  }

  # Read def and sdc
  # Use -order_wires to build wire graph
  # for antenna checker read_def -order_wires $::env(RESULTS_DIR)/6_1_fill.def
  # -order_wires flag is REQUIRED to run RCX
  if {[info exist ::env(RCX_RULES)]} {
    read_def -order_wires $::env(RESULTS_DIR)/6_1_fill.def
  } else {
    read_def $::env(RESULTS_DIR)/6_1_fill.def
  }
  read_sdc $::env(RESULTS_DIR)/6_1_fill.sdc

  set_propagated_clock [all_clocks]
} else {
  puts "Starting final report"
}


# Delete routing obstructions for final DEF
source $::env(SCRIPTS_DIR)/deleteRoutingObstructions.tcl
deleteRoutingObstructions

write_def $::env(RESULTS_DIR)/6_final.def
write_verilog $::env(RESULTS_DIR)/6_final.v

# Run extraction and STA
if {[info exist ::env(RCX_RULES)]} {

  # Set RC corner for RCX
  # Set in config.mk
  if {[info exist ::env(RCX_RC_CORNER)]} {
    set rc_corner $::env(RCX_RC_CORNER)
  }

  # Set via resistances
  source $::env(PLATFORM_DIR)/setRC.tcl

  # RCX section
  define_process_corner -ext_model_index 0 X
  extract_parasitics -ext_model_file $::env(RCX_RULES)

  # Write Spef
  write_spef $::env(RESULTS_DIR)/6_final.spef
  file delete $::env(DESIGN_NAME).totCap

  # Read Spef for OpenSTA
  read_spef $::env(RESULTS_DIR)/6_final.spef

  # Static IR drop analysis
  if {[info exist ::env(PWR_NETS_VOLTAGES)]} {
    dict for {pwrNetName pwrNetVoltage}  {*}$::env(PWR_NETS_VOLTAGES) {
        set_pdnsim_net_voltage -net ${pwrNetName} -voltage ${pwrNetVoltage}
        analyze_power_grid -net ${pwrNetName}
    }
  } else {
    puts "IR drop analysis for power nets is skipped because PWR_NETS_VOLTAGES is undefined"
  }
  if {[info exist ::env(GND_NETS_VOLTAGES)]} {
    dict for {gndNetName gndNetVoltage}  {*}$::env(GND_NETS_VOLTAGES) {
        set_pdnsim_net_voltage -net ${gndNetName} -voltage ${gndNetVoltage}
        analyze_power_grid -net ${gndNetName}
    }
  } else {
    puts "IR drop analysis for ground nets is skipped because GND_NETS_VOLTAGES is undefined"
  }

} else {
  puts "OpenRCX is not enabled for this platform."
}

source $::env(SCRIPTS_DIR)/report_metrics.tcl
report_metrics "finish"

if {![info exists standalone] || $standalone} {
  exit
}
