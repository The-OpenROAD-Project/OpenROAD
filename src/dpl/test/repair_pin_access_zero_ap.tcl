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
set other [$block findInst v]
foreach swire [[$block findNet VDD] getSWires] {
  odb::dbSWire_destroy $swire
}

# A narrow pin needs a different placement phase to reach an on-grid via.
$inst setLocation 700 0
check_placement
check "ordinary PA still rejects zero access" {
  catch {
    pin_access -via_access_layer M1 -via_in_pin_bottom_layer M1 \
      -via_in_pin_top_layer M1 -min_access_points 1 -verbose 0
  } message
} 1
check "ordinary failure is missing access" {string match "*DRT-0073*" $message} 1

check "one attempt cannot reach a viable phase" {
  catch {repair_pin_access -recovery_attempts 1} message
} 1
check "exhaustion restores the original location" {$inst getLocation} "700 0"
check "exhaustion leaves ordinary repair unready" {catch {repair_pin_access}} 1
check "zero attempts leaves recovery disabled" {
  catch {repair_pin_access -recovery_attempts 0}
} 1
check "disabled recovery preserves placement" {$inst getLocation} "700 0"
check "two attempts recover native access" {
  repair_pin_access -recovery_attempts 2
} 0
check "recovery selects the nearest viable site" {$inst getLocation} "800 0"
check "recovered native state supports ordinary repair" {repair_pin_access} 0

check "unmoved shared-class member stays in place" {$other getLocation} "4400 0"
foreach cell [list $inst $other] {
  set points [[$cell findITerm A] getPrefAccessPoints]
  check "native preferred access exists for [$cell getName]" {llength $points} 1
  check "native preferred access is local for [$cell getName]" {
    [lindex $points 0] getPoint
  } "400 500"
}
check_placement

check_error "unreachable AP minimum rejects recovery" {
  repair_pin_access -recovery_attempts 1 -min_access_points 1000
} DPL-0516
check "minimum failure restores the target" {$inst getLocation} "800 0"
check "minimum failure preserves the shared member" {$other getLocation} "4400 0"
check "minimum failure leaves PA unready" {catch {repair_pin_access}} 1
check "explicit one-AP minimum recovers again" {
  repair_pin_access -recovery_attempts 2 -min_access_points 1
} 0

foreach value {-1 wrong} {
  check_error "invalid attempt limit $value" {
    repair_pin_access -recovery_attempts $value
  } STA-0574
}
exit_summary
