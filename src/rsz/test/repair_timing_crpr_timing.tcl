# repair_timing CRPR options use the requested mode on an OCV timing fixture.

source helpers.tcl

namespace eval crpr_timing {
  variable phase_calls {}
}

proc crpr_timing::assert_equal { actual expected label } {
  if { $actual ne $expected } {
    error "$label: expected {$expected}, got {$actual}"
  }
}

proc crpr_timing::assert_near { actual expected tolerance label } {
  if { abs($actual - $expected) > $tolerance } {
    error "$label: expected $expected, got $actual"
  }
}

proc crpr_timing::phase_trace { command operation } {
  lappend ::crpr_timing::phase_calls [list [lindex $command 0] [sta::crpr_enabled]]
}

proc crpr_timing::timing_metrics { } {
  return [list \
    [sta::worst_slack -min] \
    [sta::worst_slack -max] \
    [sta::total_negative_slack -min] \
    [sta::total_negative_slack -max]]
}

proc crpr_timing::check_fresh_metrics { before after } {
  # OpenSTA timing APIs return seconds, so 0.000001 ns is 1e-15 seconds.
  set tolerance_seconds 1e-15
  foreach label {hold_wns setup_wns hold_tns setup_tns} \
      actual $before expected $after {
    assert_near $actual $expected $tolerance_seconds "fresh $label"
  }
}

proc crpr_timing::child_case { case_name } {
  read_liberty Nangate45/Nangate45_typ.lib
  read_lef Nangate45/Nangate45.lef
  read_def repair_hold1.def

  create_clock -period 0.08 clk
  set_input_delay -clock clk 0.0 {in1 in2}
  set_output_delay -clock clk -0.3 out
  set_propagated_clock clk
  set_operating_conditions -analysis_type on_chip_variation
  set_timing_derate -early 0.9
  set_timing_derate -late 1.1
  source Nangate45/Nangate45.rc
  set_wire_rc -layer metal1
  estimate_parasitics -placement

  # Prove this propagated-clock OCV fixture is CRPR-sensitive before repair.
  sta::set_crpr_enabled 1
  set crpr_on_setup [sta::worst_slack -max]
  sta::set_crpr_enabled 0
  set crpr_off_setup [sta::worst_slack -max]
  if { abs($crpr_on_setup - $crpr_off_setup) <= 1e-15 } {
    error "fixture is not CRPR-sensitive"
  }
  sta::set_crpr_enabled 1
  if { [sta::worst_slack -max] >= 0.0 || [sta::worst_slack -min] >= 0.0 } {
    error "fixture must have setup and hold violations before repair"
  }

  set options {}
  set entry_crpr 1
  set expected_phases {{rsz::repair_setup 1} {rsz::repair_hold 1}}
  set expect_error 0
  switch -- $case_name {
    default {
    }
    setup_off {
      set options {-skip_crpr_setup}
      set expected_phases {{rsz::repair_setup 0} {rsz::repair_hold 1}}
    }
    hold_off {
      set options {-skip_crpr_hold}
      set expected_phases {{rsz::repair_setup 1} {rsz::repair_hold 0}}
    }
    both_off {
      set options {-skip_crpr_setup -skip_crpr_hold}
      set expected_phases {{rsz::repair_setup 0} {rsz::repair_hold 0}}
    }
    entry_off {
      set options {-skip_crpr_setup -skip_crpr_hold}
      set entry_crpr 0
      set expected_phases {{rsz::repair_setup 0} {rsz::repair_hold 0}}
    }
    error {
      set options {-skip_crpr_setup -phases INVALID_CRPR_TEST}
      set expected_phases {{rsz::repair_setup 0}}
      set expect_error 1
    }
    default {
      error "unknown CRPR test case $case_name"
    }
  }

  sta::set_crpr_enabled $entry_crpr
  set ::crpr_timing::phase_calls {}
  trace add execution rsz::repair_setup enter crpr_timing::phase_trace
  trace add execution rsz::repair_hold enter crpr_timing::phase_trace
  try {
    if { $expect_error } {
      set status [catch {repair_timing {*}$options -max_iterations 1} message]
      assert_equal $status 1 "error status"
      if { ![string match "*RSZ-0217*" $message] } {
        error "unexpected repair error: $message"
      }
    } else {
      repair_timing {*}$options -max_iterations 1
    }
  } finally {
    trace remove execution rsz::repair_hold enter crpr_timing::phase_trace
    trace remove execution rsz::repair_setup enter crpr_timing::phase_trace
  }

  assert_equal $::crpr_timing::phase_calls $expected_phases "phase CRPR modes"
  assert_equal [sta::crpr_enabled] $entry_crpr "CRPR restoration"

  set report [make_result_file "repair_timing_crpr_${case_name}.rpt"]
  report_checks -format full_clock_expanded -path_delay max -to r3/D > $report
  set normal_metrics [timing_metrics]
  sta::find_timing_cmd true
  sta::find_requireds
  set fresh_metrics [timing_metrics]
  check_fresh_metrics $normal_metrics $fresh_metrics
  puts "case=$case_name crpr_on_setup=$crpr_on_setup crpr_off_setup=$crpr_off_setup"
  puts "metrics=$normal_metrics"
  puts "pass"
}

if { ![info exists ::env(CRPR_TEST_CASE)] } {
  set script [file normalize [info script]]
  set cases {default setup_off hold_off both_off entry_off error}
  foreach case_name $cases {
    set output [file join [make_result_dir] "repair_timing_crpr_${case_name}.log"]
    if { [catch {
      exec env CRPR_TEST_CASE=$case_name [info nameofexecutable] -no_init -no_splash -exit $script \
        > $output 2>@1
    } message] } {
      error "child $case_name failed: $message"
    }
    set channel [open $output r]
    set output_text [read $channel]
    close $channel
    if { ![string match "*pass\n" $output_text] } {
      error "child $case_name did not report pass"
    }
    puts -nonewline $output_text
  }
  puts "pass"
} else {
  crpr_timing::child_case $::env(CRPR_TEST_CASE)
}
