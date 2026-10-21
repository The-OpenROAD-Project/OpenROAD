# Check the experimental option marshaling without executing placement.
set test_name congestion_inflation_option
set ::placement_flags {}
foreach command {replace_initial_place_cmd replace_nesterov_place_cmd replace_reset_cmd} {
  rename gpl::$command gpl::saved_$command
}
proc gpl::replace_initial_place_cmd { keys flags } {
  set ::placement_flags $flags
}
proc gpl::replace_nesterov_place_cmd { keys flags } {
  if { $flags ne $::placement_flags } {
    error "placement phases received different flags"
  }
}
proc gpl::replace_reset_cmd { } { }

global_placement -timing_driven -routability_driven
array set defaults $::placement_flags
if { [info exists defaults(-timing_driven_congestion_inflation)] } {
  error "experimental congestion inflation must be opt-in"
}
global_placement -timing_driven -routability_driven -timing_driven_congestion_inflation
array set enabled $::placement_flags
if { ![info exists enabled(-timing_driven_congestion_inflation)] } {
  error "experimental congestion inflation flag was not forwarded"
}
foreach command {replace_initial_place_cmd replace_nesterov_place_cmd replace_reset_cmd} {
  rename gpl::$command {}
  rename gpl::saved_$command gpl::$command
}
puts pass
