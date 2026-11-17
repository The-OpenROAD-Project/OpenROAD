# Check normal placement flags without an extra congestion-inflation option.
set test_name congestion_inflation_default
set ::placement_calls {}
foreach command {replace_initial_place_cmd replace_nesterov_place_cmd replace_reset_cmd} {
  rename gpl::$command gpl::saved_$command
}
proc gpl::replace_initial_place_cmd { keys flags } {
  lappend ::placement_calls [list initial $keys $flags]
}
proc gpl::replace_nesterov_place_cmd { keys flags } {
  lappend ::placement_calls [list nesterov $keys $flags]
}
proc gpl::replace_reset_cmd { } {
  lappend ::placement_calls [list reset {} {}]
}

foreach flags {{} {-timing_driven} {-routability_driven} {-timing_driven -routability_driven}} {
  set ::placement_calls {}
  global_placement {*}$flags
  if { [llength $::placement_calls] != 3 } {
    error "placement must run both phases and reset between calls"
  }
  foreach call [lrange $::placement_calls 0 1] phase {initial nesterov} {
    lassign $call actual_phase keys actual_flags
    array unset forwarded
    array set forwarded $actual_flags
    if {
      $actual_phase ne $phase || $keys ne {} ||
      [lsort [array names forwarded]] ne [lsort $flags]
    } {
      error "placement phases did not receive the requested flags"
    }
  }
  if { [lindex $::placement_calls 2] ne {reset {} {}} } {
    error "placement was not reset"
  }
}
set ::placement_calls {}
if {
  ![catch { global_placement -timing_driven_congestion_inflation }] ||
  [llength $::placement_calls] != 0
} {
  error "the removed congestion-inflation flag must be rejected"
}
foreach command {replace_initial_place_cmd replace_nesterov_place_cmd replace_reset_cmd} {
  rename gpl::$command {}
  rename gpl::saved_$command gpl::$command
}
puts pass
