# Hierarchical module swaps must preserve topology and honor each timing decision.
set test_name replace_hier_mod_timing
set incremental_calls 0
set decisions {}
rename global_placement hierarchy_timing_global_placement

proc hierarchy_timing_read { path } {
  set stream [open $path r]
  set text [read $stream]
  close $stream
  return $text
}

proc hierarchy_timing_equal { actual expected label } {
  if { abs($actual - $expected) > 1e-15 } {
    error "$label is stale or does not match the selected placement: $actual vs $expected"
  }
}

proc global_placement { args } {
  global incremental_calls decisions
  if { [lsearch -exact $args -incremental] == -1 } {
    hierarchy_timing_global_placement {*}$args
    return
  }
  incr incremental_calls
  set before [make_result_file "hierarchy_timing_${incremental_calls}_before.v"]
  set after [make_result_file "hierarchy_timing_${incremental_calls}_after.v"]
  write_verilog $before
  with_output_to_variable log_text {
    hierarchy_timing_global_placement {*}$args
  }
  puts $log_text
  write_verilog $after
  if { [hierarchy_timing_read $before] != [hierarchy_timing_read $after] } {
    error "Incremental timing placement changed hierarchical connectivity or cell sizes"
  }
  set pattern {locked WS ([^,]+), TNS ([^;]+); trial WS ([^,]+), TNS ([^;]+); accepted (true|false)}
  if { ![regexp $pattern $log_text unused locked_ws locked_tns trial_ws trial_tns accepted] } {
    error "Missing locked/trial timing decision for hierarchical incremental placement"
  }
  lappend decisions $accepted
  set degraded [expr {
    $trial_ws < $locked_ws - 1e-15 || $trial_tns < $locked_tns - 1e-15
  }]
  if { ($accepted == "true") == $degraded } {
    error "Hierarchical placement decision disagrees with locked/trial setup timing"
  }
  if { $accepted == "true" } {
    set expected_ws $trial_ws
    set expected_tns $trial_tns
  } else {
    set expected_ws $locked_ws
    set expected_tns $locked_tns
  }
  hierarchy_timing_equal [sta::worst_slack_cmd max] $expected_ws "Returned setup WS"
  hierarchy_timing_equal [sta::total_negative_slack_cmd max] $expected_tns "Returned setup TNS"
  estimate_parasitics -placement
  hierarchy_timing_equal [sta::worst_slack_cmd max] $expected_ws "Fresh setup WS"
  hierarchy_timing_equal [sta::total_negative_slack_cmd max] $expected_tns "Fresh setup TNS"
  foreach inst [[ord::get_db_block] getInsts] {
    if { [$inst getPlacementStatus] == "NONE" } {
      error "Hierarchical incremental placement left a replacement cell unplaced"
    }
  }
}

# Keep the existing two-corner GPL/DPL/ODB flow intact. The default timing
# mode is intentional, not a compatibility override of the original test.
source replace_hier_mod1.tcl
if { $incremental_calls != 3 || $decisions != {true false false} } {
  error "Expected a real accepted swap followed by rollback of reverse swap and redo"
}
puts pass
