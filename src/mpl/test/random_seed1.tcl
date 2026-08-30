# Test that -random_seed makes rtl_macro_placer a deterministic candidate
# generator: the same seed must reproduce the macro placement exactly and
# a different seed must explore a different annealing trajectory.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/halos1.def"

set_thread_count 0

# Restore the macros to their pre-placement state so that the placer
# can be run repeatedly on the same design.
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

proc place_with_seed { seed placement_file initial_state } {
  restore_macro_state $initial_state
  rtl_macro_placer -random_seed $seed \
    -report_directory [make_result_dir] \
    -write_macro_placement $placement_file
}

proc placements_equal { file1 file2 } {
  set stream1 [open $file1 r]
  set content1 [read $stream1]
  close $stream1

  set stream2 [open $file2 r]
  set content2 [read $stream2]
  close $stream2

  return [expr { $content1 eq $content2 }]
}

set initial_state [save_macro_state]

set seed1_first [make_result_file random_seed1_seed1_first.tcl]
set seed1_second [make_result_file random_seed1_seed1_second.tcl]
set seed0 [make_result_file random_seed1_seed0.tcl]

place_with_seed 1 $seed1_first $initial_state
place_with_seed 1 $seed1_second $initial_state
place_with_seed 0 $seed0 $initial_state

puts "same seed is reproducible: [placements_equal $seed1_first $seed1_second]"
puts "different seed is distinct: [expr { ![placements_equal $seed1_first $seed0] }]"
