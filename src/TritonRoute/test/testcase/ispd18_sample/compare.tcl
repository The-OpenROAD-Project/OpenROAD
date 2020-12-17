#! /usr/bin/tclsh

set golden_log       [lindex $argv 0]
set test_log         [lindex $argv 1]

set test_wirelength [format "%12d" 0]
set golden_wirelength [format "%12d" 0]
set test_via [format "%12d" 0]
set golden_via [format "%12d" 0]
set test_drc [format "%12d" 0]
set golden_drc [format "%12d" 0]

if {[file exists $golden_log]} {
  set golden_wirelength [format "%12d" [exec grep -e {total wire length = } $golden_log | tail -1 | awk {{print $5}}]]
  set golden_via [format "%12d" [exec grep -e {total number of vias = } $golden_log | tail -1 | awk {{print $6}}]]
  set golden_drc [format "%12d" [exec grep -e {number of violations = } $golden_log | tail -1 | awk {{print $5}}]]
} else {
  puts "golden file $golden_log not found"
  exit 1
}

if {[file exists $test_log]} {
  set test_wirelength [format "%12d" [exec grep -e {total wire length = } $test_log | tail -1 | awk {{print $5}}]]
  set test_via [format "%12d" [exec grep -e {total number of vias = } $test_log | tail -1 | awk {{print $6}}]]
  set test_drc [format "%12d" [exec grep -e {number of violations = } $test_log | tail -1 | awk {{print $5}}]]
} else {
  puts "test file $test_log not found"
  exit 1
}

if { $golden_wirelength < $test_wirelength } {
  puts "Wire Length: golden=$golden_wirelength test=$test_wirelength"
  exit 1
}

if { $golden_via < $test_via } {
  puts "Vias: golden=$golden_via test=$test_via"
  exit 1
}

if { $golden_drc < $test_drc } {
  puts "DRC: golden=$golden_drc test=$test_drc"
  exit 1
}

exit 0
