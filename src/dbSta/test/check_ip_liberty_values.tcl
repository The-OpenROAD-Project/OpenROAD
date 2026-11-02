# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# Liberty transition tables and input capacitance are checked against the
# limits the library declares for itself.
source "helpers.tcl"

read_lef check_ip_liberty_values.lef
read_liberty check_ip_liberty_values.lib

proc expect_check_ip_pass { master_name } {
  if { [catch { check_ip -master $master_name } err] } {
    puts "FAIL: expected $master_name to pass: $err"
    exit 1
  }
}

proc expect_check_ip_fail { master_name args } {
  if { ![catch { check_ip -master $master_name {*}$args } err] } {
    puts "FAIL: expected $master_name to fail"
    exit 1
  }
}

expect_check_ip_pass lib_values_ok
expect_check_ip_fail lib_zero_transition
expect_check_ip_fail lib_long_transition
expect_check_ip_fail lib_large_input_cap

# The library limits are the default; the command line overrides them.
expect_check_ip_fail lib_values_ok -max_transition 0.01
expect_check_ip_fail lib_values_ok -max_capacitance 0.0001

puts "pass"
