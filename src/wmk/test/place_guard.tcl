# A mark that costs timing is put back rather than shipped.
#
# Swapping two cells moves their pins, which changes the wires and so the
# slack on whatever they drive.  Usually by very little -- at the default
# budget of 20 ps nothing on this design is undone -- but "usually" is not a
# guarantee, and a watermark that quietly costs a path its slack is worse than
# no watermark.
#
# The budget here is 1 ps, small enough that two pairs exceed it.  Testing at
# the default would test nothing, because a guard that never fires and a guard
# that cannot see are indistinguishable from the outside -- and this one could
# not see at all until it was given a way to re-evaluate timing.  Moving a cell
# changes only its parasitics, and nothing recomputes those until asked, so the
# check was comparing each slack against itself.
#
# Hence estimate_parasitics below: without timing set up the guard has nothing
# to compare and says so instead of pretending.
source "helpers.tcl"

foreach id { 5 6 7 8 9 392 393 1102 1103 1104 } {
  suppress_message DPL $id
}

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2.0 [get_ports clk]
set_wire_rc -signal -layer metal3
estimate_parasitics -placement

set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
set claims [make_result_file place_guard.csv]

# Compare every connected instance pin immediately after the command and
# after an independent extraction. This detects stale RC after rollback.
proc pin_slacks { } {
  set result [dict create]
  foreach pin [get_pins -hierarchical *] {
    foreach property { slack_max_rise slack_max_fall slack_min_rise slack_min_fall } {
      set slack [get_property $pin $property]
      if { abs($slack) < 1e20 } {
        dict set result "[get_full_name $pin]:$property" $slack
      }
    }
  }
  return $result
}
set before [pin_slacks]
set selected [tee -variable output [list place_watermark -key_hex $key -claims_file $claims \
  -hpwl_eps_um 1.0 -pair_dist_um 3.0 -pairs_per_tile 64 \
  -guard_degrade_ns 0.001]]
check "timing-rejected pairs remain claimed" { set selected } 24
check "this fixture exercises rollback" { string match {*WMK-0058*} $output } 1
set immediate [pin_slacks]
estimate_parasitics -placement
set fresh [pin_slacks]
set consistent 1
set safe 1
foreach key [dict keys $before] {
  if { ![dict exists $immediate $key] || ![dict exists $fresh $key] } {
    set consistent 0
    set safe 0
    continue
  }
  if { abs([dict get $immediate $key] - [dict get $fresh $key]) > 1e-6 } {
    set consistent 0
  }
  if { [dict get $fresh $key] < [dict get $before $key] - 0.001001 } {
    set safe 0
  }
}
check "timing agrees with a fresh extraction" { set consistent } 1
check "the final placement respects the timing budget" { set safe } 1
check_placement
exit_summary
