# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

source "helpers.tcl"
set_thread_count 2
read_lef repair_pin_access_tech.lef
read_lef repair_pin_access_phase.lef
read_def repair_pin_access_failure.def
set_routing_layers -signal M2-M3
check_placement
pin_access -via_access_layer M1 -via_in_pin_bottom_layer M1 \
  -via_in_pin_top_layer M1 -min_access_points 1 -verbose 0

set inst [[ord::get_db_block] findInst u]
set initial_ap [lindex [[$inst findITerm A] getPrefAccessPoints] 0]
set initial_point [$initial_ap getPoint]
check "phase-changing repair resolves conflicts" {
  repair_pin_access -max_displacement {3 1}
} 0
check_placement
lassign [$inst getLocation] ix iy
check "repair creates a new track phase" {expr {$ix % 400 != 0}} 1
set refreshed_ap [lindex [[$inst findITerm A] getPrefAccessPoints] 0]
check "native AP uses the new class" {expr {[$refreshed_ap getPoint] ne $initial_point}} 1
lassign [$refreshed_ap getPoint] ax ay
check "refreshed phase clears fixed supply via" {
  expr {abs($ix + $ax - 800) >= 220 || abs($iy + $ay - 500) >= 220}
} 1
check "phase-changing repair is stable" {repair_pin_access} 0
exit_summary
