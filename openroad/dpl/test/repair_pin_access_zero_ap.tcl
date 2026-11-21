# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

source "helpers.tcl"
set_thread_count 2
read_lef repair_pin_access_tech.lef
read_lef repair_pin_access_failure.lef
read_def repair_pin_access_failure.def
set_routing_layers -signal M2-M3
set block [ord::get_db_block]
set inst [$block findInst u]
foreach swire [[$block findNet VDD] getSWires] {
  odb::dbSWire_destroy $swire
}

# A narrow pin needs a different placement phase to reach an on-grid via.
$inst setLocation 700 0
check_placement
check_error "ordinary PA rejects zero access" {
  pin_access -via_access_layer M1 -via_in_pin_bottom_layer M1 \
    -via_in_pin_top_layer M1 -min_access_points 1 -verbose 0
} DRT-0073
check_error "disabled recovery requires published access" {
  repair_pin_access -recovery_attempts 0
} DPL-0510
check "disabled recovery preserves placement" {$inst getLocation} "700 0"
check "default recovery publishes native access" {repair_pin_access} 0
check "recovery selects the nearest viable phase" {$inst getLocation} "800 0"
check "native preferred access is published" {
  llength [[$inst findITerm A] getPrefAccessPoints]
} 1
check "published access supports ordinary repair" {repair_pin_access -recovery_attempts 0} 0
check "shared-class member stays in place" {[$block findInst v] getLocation} "4400 0"
check_placement
exit_summary
