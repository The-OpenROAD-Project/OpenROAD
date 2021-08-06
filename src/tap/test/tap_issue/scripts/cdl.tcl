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

  # Read def
  read_def $::env(RESULTS_DIR)/6_1_fill.def
} else {
  puts "Starting CDL"
}

cdl read_masters $::env(CDL_FILE)
cdl out $::env(RESULTS_DIR)/6_final.cdl

if {![info exists standalone] || $standalone} {
  exit
}
