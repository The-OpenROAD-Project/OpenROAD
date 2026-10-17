# Selective rollback can still leave another endpoint outside its budget.
# The fallback must restore the entire placement and refresh its parasitics.
source helpers.tcl
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2 [get_ports clk]
set_wire_rc -signal -layer metal3
estimate_parasitics -placement

proc placement_snapshot { } {
  set result [dict create]
  foreach inst [[ord::get_db_block] getInsts] {
    dict set result [$inst getName] \
      [list [$inst getLocation] [$inst getOrient] [$inst getPlacementStatus]]
  }
  return $result
}
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
set placement_before [placement_snapshot]
set timing_before [pin_slacks]
set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
set claims [make_result_file place_full_rollback.csv]
set count [tee -variable output [list place_watermark -key_hex $key -claims_file $claims \
  -hpwl_eps_um 1 -pair_dist_um 3 -pairs_per_tile 64 -guard_degrade_ns 0.0001]]
check "the complete-placement fallback runs" { string match {*WMK-0110*} $output } 1
check "a design that carries no mark claims nothing" { set count } 0
check "the claim file records that nothing was claimed" {
  set fh [open $claims r]
  set text [read $fh]
  close $fh
  set text
} "kind,id,A_name,B_name,target_bit,skipped_reason\n"
check "location, orientation and status are restored for every instance" {
  placement_snapshot
} $placement_before
set immediate [pin_slacks]
estimate_parasitics -placement
set fresh [pin_slacks]
set consistent [expr { [dict size $timing_before] > 0 }]
foreach pin [dict keys $timing_before] {
  if { ![dict exists $immediate $pin] || ![dict exists $fresh $pin] } {
    set consistent 0
    continue
  }
  foreach after [list $immediate $fresh] {
    if { abs([dict get $after $pin] - [dict get $timing_before $pin]) > 1e-6 } {
      set consistent 0
    }
  }
}
check "restored timing matches the original and an independent extraction" { set consistent } 1
check_placement
exit_summary
