if {![info exists standalone] || $standalone} {
  # Read lef
  read_lef $::env(TECH_LEF)
  read_lef $::env(SC_LEF)
  if {[info exist ::env(ADDITIONAL_LEFS)]} {
    foreach lef $::env(ADDITIONAL_LEFS) {
      read_lef $lef
    }
  }

  # Read def
  read_def $::env(RESULTS_DIR)/5_route.def
} else {
  puts "Starting density fill"
}

# Delete routing obstructions for final DEF
source $::env(SCRIPTS_DIR)/deleteRoutingObstructions.tcl
deleteRoutingObstructions

density_fill -rules $::env(FILL_CONFIG)

if {![info exists standalone] || $standalone} {
  write_def $::env(RESULTS_DIR)/6_1_fill.def
  write_verilog $::env(RESULTS_DIR)/6_1_fill.v
  exit
}
