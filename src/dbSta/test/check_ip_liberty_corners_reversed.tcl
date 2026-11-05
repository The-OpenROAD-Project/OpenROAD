# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# A bad transition table in an earlier corner's library is reported.
source "helpers.tcl"

define_corners typ alt
read_lef check_ip_liberty_values.lef
read_liberty -corner alt check_ip_liberty_corners.lib
read_liberty -corner typ check_ip_liberty_values.lib

set failed [catch { tee -quiet -variable log { check_ip -master lib_values_ok } }]
if { !$failed || ![string match "*CHK-0140\]*check_ip_liberty_corners*" $log] } {
  puts "FAIL: expected CHK-0140 from check_ip_liberty_corners:\n$log"
  exit 1
}

puts "pass"
