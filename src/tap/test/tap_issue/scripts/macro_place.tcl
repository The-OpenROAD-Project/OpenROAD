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
  read_def $::env(RESULTS_DIR)/2_3_floorplan_tdms.def
  read_sdc $::env(RESULTS_DIR)/1_synth.sdc
  if [file exists $::env(PLATFORM_DIR)/derate.tcl] {
    source $::env(PLATFORM_DIR)/derate.tcl
  }
} else {
  puts "Starting macro placement"
}

proc find_macros {} {
  set macros ""

  set db [ord::get_db]
  set block [[$db getChip] getBlock]
  foreach inst [$block getInsts] {
    set inst_master [$inst getMaster]

    # BLOCK means MACRO cells
    if { [string match [$inst_master getType] "BLOCK"] } {
      append macros " " $inst
    }
  }
  return $macros
}


if {[find_macros] != ""} {
  if {[info exists ::env(MACRO_PLACEMENT)]} {
    source $::env(SCRIPTS_DIR)/read_macro_placement.tcl
    puts "\[INFO\]\[FLOW-xxxx\] Using manual macro placement file $::env(MACRO_PLACEMENT)"
    read_macro_placement $::env(MACRO_PLACEMENT)
  } else {
    macro_placement \
      -halo $::env(MACRO_PLACE_HALO) \
      -channel $::env(MACRO_PLACE_CHANNEL)
  }

  if {[info exists ::env(MACRO_BLOCKAGE_HALO)]} {
    source $::env(SCRIPTS_DIR)/placement_blockages.tcl
    block_channels $::env(MACRO_BLOCKAGE_HALO)
  }
} else {
  puts "No macros found: Skipping macro_placement"
}

if {![info exists standalone] || $standalone} {
  write_def $::env(RESULTS_DIR)/2_4_floorplan_macro.def
  exit
}
