# Check worker ownership without discarding real same-net spacing violations.
source "helpers.tcl"
set test_name "check_drc_worker_boundary_${case}"
read_lef check_drc_worker_boundary_synthetic.lef
read_def "${test_name}.def"
set expected [expr { $case in {real_gap straddle} ? 1 : 0 }]
set local_box {27200 3140 27380 3240}
set outside_box {28000 3300 28500 3400}
if { $case eq "straddle" } {
  set local_box {28800 3140 28980 3240}
  set outside_box {30000 3300 30500 3400}
}

proc assert_markers { report expected label } {
  set f [open $report r]
  set text [read $f]
  close $f
  set count [regexp -all {violation type:} $text]
  if { $expected == 1 && ![string match "*violation type: Metal Spacing*on Layer metal2*" $text] } {
    error "$label: expected the genuine M2 spacing violation"
  }
  if { $count != $expected } {
    error "$label: expected $expected markers, got $count"
  }
}

foreach threads {1 4} {
  set_thread_count $threads
  if { [ord::thread_count] != $threads } { error "Unexpected thread count" }
  set report [make_result_file "${test_name}_${threads}_full.drc"]
  drt::check_drc -output_file $report
  assert_markers $report $expected full
  # check_drc's box is in DBU. The local box encloses the measured gap.
  set report [make_result_file "${test_name}_${threads}_local.drc"]
  drt::check_drc -box $local_box -output_file $report
  assert_markers $report $expected local
  set report [make_result_file "${test_name}_${threads}_outside.drc"]
  drt::check_drc -box $outside_box -output_file $report
  assert_markers $report 0 outside
}
puts "pass"
