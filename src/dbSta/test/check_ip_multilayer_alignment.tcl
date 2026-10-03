# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

source "helpers.tcl"

read_lef check_ip_multilayer_alignment.lef
read_def check_ip_multilayer_alignment.def

if { [catch { check_ip -master cross_layer_pass } err] } {
  puts "FAIL: expected cross_layer_pass to pass: $err"
  exit 1
}

if { ![catch { check_ip -master cross_layer_fail } err] } {
  puts "FAIL: expected cross_layer_fail to fail"
  exit 1
}

puts "pass"
