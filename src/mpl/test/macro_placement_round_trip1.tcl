# Test that the file written by -write_macro_placement round-trips:
# place_macro must accept every position the macro placer committed
# (no MPL-0034 out-of-core rejection from snap residue) and land every
# macro exactly where the placer left it.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/halos1.def"

set_thread_count 0

proc save_macro_state { } {
  set state {}
  foreach inst [[ord::get_db_block] getInsts] {
    if { [[$inst getMaster] isBlock] } {
      lappend state $inst [$inst getPlacementStatus] [$inst getOrient] \
        [$inst getLocation]
    }
  }
  return $state
}

proc restore_macro_state { state } {
  foreach { inst status orient location } $state {
    $inst setPlacementStatus $status
    $inst setOrient $orient
    $inst setLocation [lindex $location 0] [lindex $location 1]
  }

  # Remove the soft blockages created around the macros placed by
  # the previous run.
  foreach blockage [[ord::get_db_block] getBlockages] {
    odb::dbBlockage_destroy $blockage
  }
}

proc macro_locations { } {
  set locations {}
  foreach inst [[ord::get_db_block] getInsts] {
    if { [[$inst getMaster] isBlock] } {
      lappend locations [$inst getName] [$inst getLocation] [$inst getOrient]
    }
  }
  return $locations
}

set initial_state [save_macro_state]

set placement_file [make_result_file macro_placement_round_trip1.tcl]
rtl_macro_placer -report_directory [make_result_dir] \
  -write_macro_placement $placement_file

set placed [macro_locations]

restore_macro_state $initial_state
source $placement_file

set injected [macro_locations]

puts "round trip is exact: [expr { $placed eq $injected }]"
