#!/bin/sh
# The next line is executed by /bin/sh, but not Tcl \
exec tclsh $0 ${1+"$@"}

package require json

# Directory containing tests.
set test_dir [file dirname [file normalize [info script]]]
set openroad_dir [file dirname $test_dir]

source [file join $test_dir "regression.tcl"]
source [file join $test_dir "regression_tests.tcl"]

set result_dir [file join $test_dir "results"]

set metrics_keys {
  "IFP::instance_count"
  "ORD::design_area"
  "ORD::utilization"
  "STA::worst_slack_min"
  "STA::worst_slack_max"
  "STA::tns_max"
  "DPL::errors"
  "ANT::errors"
  "DRT::drv"
}

# short names for headers (same order as metrics_keys)
set keys_names {
  "instances"
  "area"
  "util"
  "slack_min"
  "slack_max"
  "tns_max"
  "DPL"
  "ANT"
  "drv"
}

proc report_metrics_header {} {
  global keys_names

  puts -nonewline [format "%-20s" ""]
  foreach key $keys_names {
    puts -nonewline [format "%10s" $key]
  }
  puts ""
}

proc report_test_metrics { test } {
  global metrics_keys

  set metrics_file [test_metrics_file $test]
  if { [file exists $metrics_file] } {
    set stream [open $metrics_file r]
    set json_string [read $stream]
    close $stream
    puts -nonewline [format "%-20s" $test]
    if { ![catch {json::json2dict $json_string} metrics_dict] } {
      foreach key $metrics_keys {
        if { [dict exists $metrics_dict $key] } {
          set value [dict get $metrics_dict $key]
        } else {
          set value "N/A"
        }

        if { [string is integer $value] } {
          set value [format "%10d" $value]
        } elseif { [string is double $value] } {
          set value [format "%10.3f" $value]
        } else {
          set value [format "%10s" $value]
        }
        puts -nonewline $value
      }
    }
    puts ""
  }
}

proc test_metrics_file { test } {
  global result_dir
  return [file join $result_dir "$test.metrics"]
}

if { $argc == 0 } {
  set tests $test_groups(flow)
} else {
  set tests $argv
}

report_metrics_header
foreach test $tests {
  report_test_metrics $test
}

# Local Variables:
# mode:tcl
# End:
