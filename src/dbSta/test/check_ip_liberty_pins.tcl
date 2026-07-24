# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

source "helpers.tcl"

read_lef check_ip_liberty_pins.lef

proc expect_check_ip_pass { master_name } {
  if { [catch { check_ip -master $master_name } err] } {
    puts "FAIL: expected $master_name to pass: $err"
    exit 1
  }
}

proc expect_check_ip_fail { master_name } {
  if { ![catch { check_ip -master $master_name } err] } {
    puts "FAIL: expected $master_name to fail"
    exit 1
  }
}

expect_check_ip_pass lef_lib_cell_missing

read_liberty check_ip_liberty_pins.lib

expect_check_ip_pass lef_lib_pins_match
expect_check_ip_fail lef_lib_pin_missing
expect_check_ip_fail lef_lib_cell_missing
expect_check_ip_fail lef_lib_direction_mismatch

puts "pass"
