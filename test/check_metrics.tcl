## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2026, The OpenROAD Authors

# Compare a flow test's result metrics against its metrics limits and exit
# non-zero when a metric is outside its limit. regression_test.sh runs this
# after the flow, which is what makes bazel check flow QoR; ./regression
# instead calls check_test_metrics directly.
#
# The file paths come from the environment rather than argv because this runs
# under the openroad binary, which consumes its own command line.

set metrics_dir [file dirname [file normalize [info script]]]
source [file join $metrics_dir "flow_metrics.tcl"]

foreach var {METRICS_FILE METRICS_LIMITS_FILE} {
  if { ![info exists ::env($var)] || $::env($var) == "" } {
    puts "Error: $var is not set."
    exit 1
  }
}

set result [compare_metrics_files $::env(METRICS_FILE) \
  $::env(METRICS_LIMITS_FILE)]
if { $result != "pass" } {
  puts "Metrics do not satisfy limits: $result"
  exit 1
}
puts "Metrics satisfy limits."
exit 0
