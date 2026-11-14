# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# Liberty transition tables and input capacitance are checked against the
# limits the library declares for itself.
source "helpers.tcl"

read_lef check_ip_liberty_values.lef
read_liberty check_ip_liberty_values.lib

proc run_check_ip { args } {
  set failed [catch { tee -quiet -variable log [list check_ip {*}$args] }]
  return [list $failed $log]
}

proc expect_check_ip_pass { master_name args } {
  lassign [run_check_ip -master $master_name {*}$args] failed log
  if { $failed } {
    puts "FAIL: expected $master_name to pass:\n$log"
    exit 1
  }
}

# Fails only when the expected CHK message is the one reported.
proc expect_check_ip_fail { master_name id args } {
  lassign [run_check_ip -master $master_name {*}$args] failed log
  if { !$failed } {
    puts "FAIL: expected $master_name to fail"
    exit 1
  }
  if { ![string match "*CHK-$id\]*" $log] } {
    puts "FAIL: expected CHK-$id for $master_name:\n$log"
    exit 1
  }
}

expect_check_ip_pass lib_values_ok
expect_check_ip_fail lib_zero_transition 0140
expect_check_ip_fail lib_scalar_zero 0140
expect_check_ip_fail lib_long_transition 0141
expect_check_ip_fail lib_scalar_long 0141
expect_check_ip_fail lib_just_above_limit 0141
expect_check_ip_fail lib_large_input_cap 0150
expect_check_ip_fail lib_large_inout_cap 0150
# Rounding in the written value is not reported.
expect_check_ip_pass lib_rounding_at_limit
# A three-state disable arc's zero transition is not reported.
expect_check_ip_pass lib_tristate_disable
# A pin's own max_transition takes precedence over the library default.
expect_check_ip_pass lib_pin_max_transition

# The library limits are the default; the command line overrides them.
expect_check_ip_fail lib_values_ok 0141 -max_transition 0.01
expect_check_ip_fail lib_pin_max_transition 0141 -max_transition 1.0
expect_check_ip_fail lib_values_ok 0150 -max_capacitance 0.0001

# -all reports every bad cell and nothing from the good ones.
lassign [run_check_ip -all] failed log
if { !$failed } {
  puts "FAIL: expected check_ip -all to fail"
  exit 1
}
foreach id {0140 0141 0150} {
  if { ![string match "*CHK-$id\]*" $log] } {
    puts "FAIL: expected CHK-$id from check_ip -all:\n$log"
    exit 1
  }
}
foreach master_name {lib_values_ok lib_rounding_at_limit lib_tristate_disable
  lib_pin_max_transition} {
  if { [string match "* $master_name *" $log] } {
    puts "FAIL: check_ip -all reported $master_name:\n$log"
    exit 1
  }
}

puts "pass"
