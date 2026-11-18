# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

source "helpers.tcl"
set_thread_count 2
read_lef repair_pin_access_tech.lef
read_lef repair_pin_access_failure.lef
read_def repair_pin_access_failure.def
set_routing_layers -signal M2-M3
check_placement
pin_access -via_access_layer M1 -via_in_pin_bottom_layer M1 \
  -via_in_pin_top_layer M1 -min_access_points 1 -verbose 0
set block [ord::get_db_block]
set inst [$block findInst u]
set initial_ap [lindex [[$inst findITerm A] getPrefAccessPoints] 0]
set initial_point [$initial_ap getPoint]

# Off-phase sites cannot provide an AP for the narrow pin.
check "bounded failure rejects sites without native access" {
  repair_pin_access -max_displacement {3 0}
} 2
check "bounded failure preserves first cell" {$inst getLocation} "400 0"
check "bounded failure preserves second cell" {[$block findInst v] getLocation} "4400 0"
check "candidate queries preserve published preferred AP" {
  [lindex [[$inst findITerm A] getPrefAccessPoints] 0] getPoint
} $initial_point
check "repeated bounded failure remains usable" {
  repair_pin_access -max_displacement {3 0}
} 2
check_placement
exit_summary
