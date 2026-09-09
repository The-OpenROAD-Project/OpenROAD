# repair_timing CRPR options preserve the entry setting across phase dispatch.

namespace eval crpr_options {
  variable setter_calls {}
  variable phase_calls {}
  variable fail_phase ""
  variable setup_result 1
  variable hold_result 0
  variable recover_power_result 1
}

proc crpr_options::assert_equal { actual expected label } {
  if { $actual ne $expected } {
    error "$label: expected {$expected}, got {$actual}"
  }
}

proc crpr_options::record_phase { phase } {
  variable phase_calls
  lappend phase_calls [list $phase [sta::crpr_enabled] $::sta_crpr_enabled]
}

proc crpr_options::expected_setters { entry setup hold skip_setup skip_hold } {
  set current $entry
  set setters {}
  foreach {name enabled} [list setup $setup hold $hold] {
    if { !$enabled } {
      continue
    }
    if { $name eq "setup" } {
      set target [expr {$entry && !$skip_setup}]
    } else {
      set target [expr {$entry && !$skip_hold}]
    }
    if { $current != $target } {
      lappend setters $target
      set current $target
    }
  }
  if { $current != $entry } {
    lappend setters $entry
  }
  return $setters
}

proc crpr_options::run_case { entry setup hold skip_setup skip_hold } {
  variable setter_calls
  variable phase_calls
  variable fail_phase
  variable setup_result
  variable hold_result

  set ::sta_crpr_enabled $entry
  set setter_calls {}
  set phase_calls {}
  set fail_phase ""
  set setup_result 1
  set hold_result 0

  set options {}
  if { $setup } {
    lappend options -setup
  }
  if { $hold } {
    lappend options -hold
  }
  if { $skip_setup } {
    lappend options -skip_crpr_setup
  }
  if { $skip_hold } {
    lappend options -skip_crpr_hold
  }

  set result [repair_timing {*}$options]
  set expected_phases {}
  if { $setup || (!$setup && !$hold) } {
    lappend expected_phases [list setup [expr {$entry && !$skip_setup}] \
      [expr {$entry && !$skip_setup}]]
  }
  if { $hold || (!$setup && !$hold) } {
    lappend expected_phases [list hold [expr {$entry && !$skip_hold}] \
      [expr {$entry && !$skip_hold}]]
  }
  if { !$setup && !$hold } {
    set setup 1
    set hold 1
  }
  assert_equal $phase_calls $expected_phases "phase modes"
  assert_equal $setter_calls [expected_setters $entry $setup $hold $skip_setup $skip_hold] \
    "setter sequence"
  assert_equal [sta::crpr_enabled] $entry "getter restoration"
  assert_equal $::sta_crpr_enabled $entry "trace restoration"
  assert_equal $result $setup "repair result"
}

rename sta::set_crpr_enabled sta::set_crpr_enabled_real
proc sta::set_crpr_enabled { enabled } {
  lappend ::crpr_options::setter_calls $enabled
  sta::set_crpr_enabled_real $enabled
}

rename rsz::repair_setup rsz::repair_setup_real
proc rsz::repair_setup { args } {
  crpr_options::record_phase setup
  if { $::crpr_options::fail_phase eq "setup" } {
    return -code error -errorcode {CRPR TEST SETUP} "injected setup failure"
  }
  return $::crpr_options::setup_result
}

rename rsz::repair_hold rsz::repair_hold_real
proc rsz::repair_hold { args } {
  crpr_options::record_phase hold
  if { $::crpr_options::fail_phase eq "hold" } {
    return -code error -errorcode {CRPR TEST HOLD} "injected hold failure"
  }
  return $::crpr_options::hold_result
}

rename rsz::recover_power rsz::recover_power_real
proc rsz::recover_power { args } {
  return $::crpr_options::recover_power_result
}

rename est::check_parasitics est::check_parasitics_real
proc est::check_parasitics { } {
}

rename design_is_routed design_is_routed_real
proc design_is_routed { } {
  return 0
}

rename rsz::set_max_utilization rsz::set_max_utilization_real
proc rsz::set_max_utilization { utilization } {
}

try {
  # Cover entry ON/OFF, all flag combinations, and all phase selections.
  foreach entry {0 1} {
    foreach flags {{0 0} {1 0} {0 1} {1 1}} {
      lassign $flags skip_setup skip_hold
      foreach phases {{0 0} {1 0} {0 1} {1 1}} {
        lassign $phases setup hold
        crpr_options::run_case $entry $setup $hold $skip_setup $skip_hold
      }
    }
  }

  # Power recovery does not dispatch repair phases or change CRPR.
  set ::sta_crpr_enabled 1
  set ::crpr_options::setter_calls {}
  set ::crpr_options::phase_calls {}
  crpr_options::assert_equal [repair_timing -recover_power 20 -skip_crpr_setup -skip_crpr_hold] 1 \
    "power recovery result"
  crpr_options::assert_equal $::crpr_options::phase_calls {} "power recovery phases"
  crpr_options::assert_equal $::crpr_options::setter_calls {} "power recovery setters"

  # An ordinary setup error restores CRPR and prevents hold dispatch.
  set ::sta_crpr_enabled 1
  set ::crpr_options::setter_calls {}
  set ::crpr_options::phase_calls {}
  set ::crpr_options::fail_phase setup
  set code [catch {repair_timing -skip_crpr_setup} message options]
  crpr_options::assert_equal $code 1 "setup error status"
  crpr_options::assert_equal $message "injected setup failure" "setup error message"
  crpr_options::assert_equal [dict get $options -errorcode] {CRPR TEST SETUP} "setup error code"
  crpr_options::assert_equal $::crpr_options::phase_calls {{setup 0 0}} "setup error phases"
  crpr_options::assert_equal $::crpr_options::setter_calls {0 1} "setup error setters"
  crpr_options::assert_equal [sta::crpr_enabled] 1 "setup error restoration"

  # An ordinary hold error restores CRPR after successful setup dispatch.
  set ::sta_crpr_enabled 1
  set ::crpr_options::setter_calls {}
  set ::crpr_options::phase_calls {}
  set ::crpr_options::fail_phase hold
  set code [catch {repair_timing -skip_crpr_hold} message options]
  crpr_options::assert_equal $code 1 "hold error status"
  crpr_options::assert_equal $message "injected hold failure" "hold error message"
  crpr_options::assert_equal [dict get $options -errorcode] {CRPR TEST HOLD} "hold error code"
  crpr_options::assert_equal $::crpr_options::phase_calls {{setup 1 1} {hold 0 0}} "hold error phases"
  crpr_options::assert_equal $::crpr_options::setter_calls {0 1} "hold error setters"
  crpr_options::assert_equal [sta::crpr_enabled] 1 "hold error restoration"

  # Unknown options fail before phase dispatch or a CRPR transition.
  set ::crpr_options::fail_phase ""
  set ::crpr_options::setter_calls {}
  set ::crpr_options::phase_calls {}
  set code [catch {repair_timing -unknown_crpr_option} message]
  crpr_options::assert_equal $code 1 "unknown option status"
  crpr_options::assert_equal $::crpr_options::phase_calls {} "unknown option phases"
  crpr_options::assert_equal $::crpr_options::setter_calls {} "unknown option setters"

  # A false repair result is returned unchanged when no violation is repaired.
  set ::sta_crpr_enabled 1
  set ::crpr_options::setter_calls {}
  set ::crpr_options::phase_calls {}
  set ::crpr_options::setup_result 0
  set ::crpr_options::hold_result 0
  crpr_options::assert_equal [repair_timing -setup] 0 "false repair result"
  crpr_options::assert_equal $::crpr_options::setter_calls {} "false repair setters"

  puts "pass"
} finally {
  rename rsz::set_max_utilization {}
  rename rsz::set_max_utilization_real rsz::set_max_utilization
  rename design_is_routed {}
  rename design_is_routed_real design_is_routed
  rename est::check_parasitics {}
  rename est::check_parasitics_real est::check_parasitics
  rename rsz::recover_power {}
  rename rsz::recover_power_real rsz::recover_power
  rename rsz::repair_hold {}
  rename rsz::repair_hold_real rsz::repair_hold
  rename rsz::repair_setup {}
  rename rsz::repair_setup_real rsz::repair_setup
  rename sta::set_crpr_enabled {}
  rename sta::set_crpr_enabled_real sta::set_crpr_enabled
}
