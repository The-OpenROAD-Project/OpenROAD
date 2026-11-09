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

proc full_pin_access { } {
  pin_access -via_access_layer M1 -via_in_pin_bottom_layer M1 \
    -via_in_pin_top_layer M1 -min_access_points 1 -verbose 0
}

check_error "reject before pin access" {repair_pin_access} DPL-0510
check "missing PA preserves placement" {$inst getLocation} "0 0"
full_pin_access
check "zero displacement preserves unresolved cells" {
  repair_pin_access -max_displacement {0 0}
} 2
check "zero displacement preserves placement" {$inst getLocation} "0 0"
check "repair resolves fixed supply escape conflicts" {repair_pin_access} 0
set repaired_location [$inst getLocation]
set second_location [$second_inst getLocation]
check "repair moves first affected cell" {expr {$repaired_location ne "0 0"}} 1
check "repair moves second affected cell" {expr {$second_location ne "4000 0"}} 1
check_placement
lassign $repaired_location ix iy
set aps [[$inst findITerm A] getPrefAccessPoints]
check "refreshed preferred witness" {llength $aps} 1
lassign [[lindex $aps 0] getPoint] ax ay
check "first-hop witness clears fixed via" {
  expr {abs($ix + $ax - 400) >= 220 || abs($iy + $ay - 500) >= 220}
} 1
check "clean repair result" {repair_pin_access} 0
check "clean repair preserves placement" {$inst getLocation} $repaired_location
check "clean repair preserves second cell" {$second_inst getLocation} $second_location

$inst setLocation 0 0
$inst setPlacementStatus FIRM
full_pin_access
check "fixed conflict remains unresolved" {repair_pin_access} 1
check "fixed instance does not move" {$inst getLocation} "0 0"

foreach {arguments error_id} {
  {-max_displacement 1} DPL-0350
  {-max_displacement {1 -1}} DPL-0349
  {-max_displacement {1 1.5}} DPL-0349
  {-max_rounds 0} DPL-0349
  {-max_rounds wrong} DPL-0349
} {
  check_error "invalid repair arguments $arguments" {repair_pin_access {*}$arguments} $error_id
}

set saved_db [make_result_file repair_pin_access.odb]
write_db $saved_db
odb::dbChip_destroy [[ord::get_db] getChip]
read_db $saved_db
check_error "reject loaded stale pin access" {repair_pin_access} DPL-0510
exit_summary
