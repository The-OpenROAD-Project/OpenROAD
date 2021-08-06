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

  # Read design files
  read_def $::env(RESULTS_DIR)/4_1_cts.def
} else {
  puts "Starting fill cell"
}

filler_placement $::env(FILL_CELLS)
check_placement

if {![info exists standalone] || $standalone} {
  # write output
  write_def $::env(RESULTS_DIR)/4_2_cts_fillcell.def
  exit
} else {
  # FIXME: TritonRoute still requires this for .def.ref hack
  write_def $::env(RESULTS_DIR)/4_cts.def
}
