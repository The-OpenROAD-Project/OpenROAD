# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# LEF-CHK-003 cross-layer alignment: M1 and M3 are both horizontal, so one
# macro Y translation must put the M1 and M3 signal pins on their tracks.
# Vertical layers constrain X; mixed directions constrain separate axes.
source "helpers.tcl"

read_lef check_ip_multilayer_alignment.lef
read_def check_ip_multilayer_alignment.def

# Run check_ip on one master and print whether it passed. A failing check
# logs its CHK-0030/CHK-0031 warnings and the CHK-0003 summary above the
# result line.
proc report_check_ip { master_name } {
  set failed [catch { check_ip -master $master_name }]
  puts "check_ip $master_name: [expr { $failed ? "FAIL" : "PASS" }]"
}

# M1 and M3 both need a Y translation of 150 mod 200.
report_check_ip cross_layer_pass

# M1 needs 150 mod 200 and M3 needs 50 mod 200, so no shared translation.
report_check_ip cross_layer_fail

# Vertical M2 and M4 use the X translation for compatibility.
report_check_ip vertical_pass
report_check_ip vertical_fail

# M1 and M2 require different offsets on independent axes.
report_check_ip mixed_direction_pass
