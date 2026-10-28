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
proc pin_points {block} {
  set points {}
  foreach ap [$block getAccessPoints] {
    if {[$ap getMPin] ne "NULL"} {
      lappend points [$ap getPoint]
    }
  }
  return [lsort $points]
}
set original_points [pin_points $block]

# The only trial that clears the fixed via has no AP in its new track class.
check "dirty-class failure reaches Tcl" {
  catch {repair_pin_access -max_displacement {3 1}} message
} 1
check "native pin access reports the failed class" {
  string match "*DRT-0073*" $message
} 1
check "failed refresh restores first cell" {[$block findInst u] getLocation} "400 0"
check "failed refresh restores second cell" {[$block findInst v] getLocation} "4400 0"
check_placement
check "rollback preserves the AP catalogue immediately" {pin_points $block} $original_points
foreach name {u v} {
  set points [[[$block findInst $name] findITerm A] getPrefAccessPoints]
  check "rollback preserves preferred AP count for $name" {llength $points} 1
  check "rollback preserves local preferred AP for $name" {
    [lindex $points 0] getPoint
  } "400 500"
}
check "repeated failure still reaches Tcl" {
  catch {repair_pin_access -max_displacement {3 1}} message
} 1
check "successful rollback restores readiness" {string match "*DRT-0073*" $message} 1
check "repeated rollback preserves the AP catalogue" {pin_points $block} $original_points
exit_summary
