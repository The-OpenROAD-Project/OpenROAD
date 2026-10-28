# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

source "helpers.tcl"
set_thread_count 2
read_lef repair_pin_access_tech.lef
read_lef repair_pin_access.lef
read_def repair_pin_access.def
set_routing_layers -signal M2-M3
check_placement
set block [ord::get_db_block]
set inst [$block findInst u]
set second_inst [$block findInst v]

proc full_pin_access {} {
  pin_access -via_access_layer M1 -via_in_pin_bottom_layer M1 \
    -via_in_pin_top_layer M1 -min_access_points 1 -verbose 0
}

# Native PA accepts the AP although its first-hop landing overlaps the PDN via.
check_error "reject before pin access" {repair_pin_access} DPL-0510
check "missing PA preserves placement" {$inst getLocation} "0 0"
full_pin_access
set aps [[$inst findITerm A] getPrefAccessPoints]
check "native PA has a witness" {llength $aps} 1
check "first-hop trap point" {[lindex $aps 0] getPoint} "400 500"
check "repair resolves first-hop trap" {repair_pin_access} 0
set repaired_location [$inst getLocation]
check "repair moves affected cell" {expr {$repaired_location ne "0 0"}} 1
set second_location [$second_inst getLocation]
check "repair moves second affected cell" {expr {$second_location ne "4000 0"}} 1
check_placement

# Inspect the refreshed witness at the new origin, independently of the result.
lassign $repaired_location ix iy
set aps [[$inst findITerm A] getPrefAccessPoints]
check "refreshed preferred witness" {llength $aps} 1
lassign [[lindex $aps 0] getPoint] ax ay
check "first-hop witness clears fixed via" {
  expr {abs($ix + $ax - 400) >= 220 || abs($iy + $ay - 500) >= 220}
} 1
check "clean repair result" {repair_pin_access} 0
check "clean repair is a no-op" {$inst getLocation} $repaired_location
check "clean repair preserves second cell" {$second_inst getLocation} $second_location

$inst setLocation 0 0
$inst setPlacementStatus FIRM
full_pin_access
check "fixed conflict remains unresolved" {repair_pin_access} 1
check "fixed instance does not move" {$inst getLocation} "0 0"

foreach {arguments error_id} {
  {-max_displacement 1} DPL-0350
  {-max_displacement {0 1}} DPL-0349
  {-max_displacement {1 -1}} DPL-0349
  {-max_displacement {1 1.5}} DPL-0349
  {-max_rounds 0} DPL-0349
  {-max_rounds wrong} DPL-0349
  {-min_access_points 0} DPL-0349
  {-min_access_points -1} DPL-0349
  {-min_access_points 1.5} DPL-0349
  {-min_access_points wrong} DPL-0349
} {
  check_error "invalid repair arguments $arguments" {repair_pin_access {*}$arguments} $error_id
}

# Serialized APs cannot replace the live native state needed by dirty refresh.
set saved_db [make_result_file repair_pin_access.odb]
write_db $saved_db
odb::dbChip_destroy [[ord::get_db] getChip]
read_db $saved_db
set block [ord::get_db_block]
set inst [$block findInst u]
check_error "reject loaded stale pin access" {repair_pin_access} DPL-0510
check "stale PA rejection preserves placement" {$inst getLocation} "0 0"
full_pin_access
check "fresh pin access after reload" {repair_pin_access} 1

exit_summary
