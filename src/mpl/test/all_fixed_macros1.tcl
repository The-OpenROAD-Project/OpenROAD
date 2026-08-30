# Test that a design whose macros are all fixed still gets the
# temporary standard-cell placement and the soft blockages around the
# macros, exactly like a run with unfixed macros would produce them.
# This keeps a placement re-injected through place_macro (e.g. with
# MACRO_PLACEMENT_TCL) equivalent to the run that generated it.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/fixed_macros1.def"

set_thread_count 0

set block [ord::get_db_block]
foreach inst [$block getInsts] {
  if { [[$inst getMaster] isBlock] } {
    $inst setPlacementStatus LOCKED
  }
}

rtl_macro_placer -report_directory [make_result_dir]

set placed 0
set unplaced 0
foreach inst [$block getInsts] {
  if { ![[$inst getMaster] isBlock] } {
    if { [$inst isPlaced] } {
      incr placed
    } else {
      incr unplaced
    }
  }
}

set macros_moved 0
foreach inst [$block getInsts] {
  if { [[$inst getMaster] isBlock] } {
    if { [$inst getPlacementStatus] ne "LOCKED" } {
      incr macros_moved
    }
  }
}

puts "std cells placed: $placed unplaced: $unplaced"
puts "macros disturbed: $macros_moved"
puts "blockages: [llength [$block getBlockages]]"
